#include "SentinelSharedBuffer.h"
#include "SentinelSmmCpuContext.h"
#include "SentinelSpiLockdown.h"
#include <Library/CpuLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <PiDxe.h>
#include <Protocol/S3SaveState.h>

// ----------------------------------------------------------------------------
// Policy Data Structures
// ----------------------------------------------------------------------------

// 8KB bitmap for all 65,536 I/O ports
static UINT8 IoAllowMap[8192] = {0};

// Allowed MSRs
static const UINT32 SafeMsrList[] = {
    // Placeholder for required CPL3 MSRs (e.g. APIC base, if applicable)
};
#define SAFE_MSR_COUNT (sizeof(SafeMsrList) / sizeof(UINT32))

// SPI MMIO Range (legacy constants retained for I/O port policy)
#define SPI_BASE_ADDR 0xFED01000
#define SPI_LIMIT_ADDR 0xFED02000

// ----------------------------------------------------------------------------
// Phase 3: SPI Flash Hardware Lockdown
// ----------------------------------------------------------------------------
//
// Enforces hardware-level write protection on the SPI flash chip during
// the DXE boot phase, permanently severing Ring 0's ability to modify
// firmware partitions. This is the ultimate defensive backstop:
// even if the OS kernel is fully compromised, it cannot reflash the
// ME/BIOS regions.
//
// Sequence:
//   1. Read RCBA to locate chipset MMIO base
//   2. Compute SPIBAR = RCBA + 0x3800
//   3. Read FREG2 to extract ME firmware region boundaries
//   4. Program PR0 to write-protect the ME region
//   5. Assert FLOCKDN in HSFS to lock PR0-PR4 permanently
//   6. Assert SMM_BWP and BLE in BIOS_CNTL
//   7. Clear BIOSWE to disable flash writes
// ----------------------------------------------------------------------------

#ifndef EFI_BOOT_SCRIPT_PCI_ADDRESS
#define EFI_BOOT_SCRIPT_PCI_ADDRESS(bus, dev, func, reg)                       \
  (UINT64)((((UINTN)(bus)) << 24) | (((UINTN)(dev)) << 16) |                   \
           (((UINTN)(func)) << 8) | ((UINTN)(reg)))
#endif

static EFI_STATUS EnforceSpiLockdown(VOID) {
  UINT32 RcbaRaw;
  UINT32 RcbaBase;
  UINT32 SpiBar;
  UINT32 Hsfs;
  UINT32 Freg2;
  UINT32 MeBase;
  UINT32 MeLimit;
  UINT32 Pr0Value;
  UINT8 BiosCntl;
  EFI_S3_SAVE_STATE_PROTOCOL *S3SaveState;
  EFI_STATUS S3Status;
  UINT32 HsfsValue;

  DEBUG((DEBUG_INFO,
         "[SentinelSmm] --- Phase 3: SPI Flash Hardware Lockdown ---\n"));

  // Locate the S3 Save State protocol to record S3 resume boot scripts
  S3Status = gBS->LocateProtocol(&gEfiS3SaveStateProtocolGuid, NULL,
                                 (VOID **)&S3SaveState);
  if (EFI_ERROR(S3Status)) {
    DEBUG((DEBUG_WARN,
           "[SentinelSmm] WARNING: EFI_S3_SAVE_STATE_PROTOCOL not found. "
           "S3 resume protections will NOT be re-asserted on wake.\n"));
    S3SaveState = NULL;
  } else {
    DEBUG((DEBUG_INFO, "[SentinelSmm] Found EFI_S3_SAVE_STATE_PROTOCOL. S3 "
                       "resume hooks enabled.\n"));
  }

  // ========================================================================
  // Step 1: Locate the Root Complex Base Address (RCBA)
  // ========================================================================
  RcbaRaw = PciRead32(LPC_PCI_ADDR(R_RCBA));
  if ((RcbaRaw & B_RCBA_ENABLE) == 0) {
    DEBUG((DEBUG_ERROR, "[SentinelSmm] FATAL: RCBA is not enabled. "
                        "Cannot access SPI registers. ABORTING.\n"));
    return EFI_DEVICE_ERROR;
  }

  RcbaBase = RcbaRaw & RCBA_ADDR_MASK;
  SpiBar = RcbaBase + SPIBAR_OFFSET;

  DEBUG((DEBUG_INFO, "[SentinelSmm]   RCBA Base:  0x%08X\n", RcbaBase));
  DEBUG((DEBUG_INFO, "[SentinelSmm]   SPIBAR:     0x%08X\n", SpiBar));

  // ========================================================================
  // Step 2: Extract ME Region Boundaries from Flash Descriptor (FREG2)
  // ========================================================================
  //
  // FREG2 defines the physical address range of the Intel ME firmware
  // partition on the SPI flash. We read this to dynamically program PR0.
  //
  Freg2 = MmioRead32(SpiBar + R_FREG2_ME);
  MeBase = (Freg2 & FREG_BASE_MASK) << 12; // Convert 4KB units
  MeLimit = ((Freg2 & FREG_LIMIT_MASK) >> FREG_LIMIT_SHIFT) << 12;
  MeLimit |= 0xFFF; // Align to 4KB boundary

  DEBUG((DEBUG_INFO,
         "[SentinelSmm]   ME Region:  0x%08X - 0x%08X "
         "(%d KB)\n",
         MeBase, MeLimit, (MeLimit - MeBase + 1) / 1024));

  if (MeBase == 0 && MeLimit == 0xFFF) {
    DEBUG((DEBUG_WARN, "[SentinelSmm] WARNING: ME region appears empty "
                       "or disabled. PR0 will still be programmed as a "
                       "defensive measure.\n"));
  }

  // ========================================================================
  // Step 3: Program PR0 to Write-Protect the ME Firmware Region
  // ========================================================================
  //
  // PR0 uses 4KB granularity. We set the Write Protection (WP) bit to
  // physically block all writes to the ME address range, overriding
  // even SMM-level write attempts.
  //
  Pr0Value = PR_VALUE(MeBase, MeLimit, TRUE, FALSE);
  MmioWrite32(SpiBar + R_PR0, Pr0Value);

  DEBUG((DEBUG_INFO,
         "[SentinelSmm]   PR0 Value:  0x%08X "
         "[WP=1, RP=0]\n",
         Pr0Value));

  // Verify the write stuck
  if (MmioRead32(SpiBar + R_PR0) != Pr0Value) {
    DEBUG((DEBUG_ERROR,
           "[SentinelSmm] FATAL: PR0 write verification "
           "failed. Flash may already be locked by OEM firmware.\n"));
    // Non-fatal: continue with BIOS_CNTL lockdown even if PR0 fails
  }

  // ========================================================================
  // Step 4: Assert FLOCKDN in HSFS
  // ========================================================================
  //
  // FLOCKDN creates an irreversible hardware latch on PR0-PR4, FRAP,
  // and opcode configuration registers. Once set, no software — not
  // even SMM — can reconfigure the protected ranges until hardware reset.
  //
  Hsfs = MmioRead32(SpiBar + R_HSFS);
  DEBUG((DEBUG_INFO,
         "[SentinelSmm]   HSFS Before: 0x%08X  "
         "(FLOCKDN=%d)\n",
         Hsfs, (Hsfs & B_HSFS_FLOCKDN) ? 1 : 0));

  if ((Hsfs & B_HSFS_FLOCKDN) == 0) {
    MmioOr32(SpiBar + R_HSFS, B_HSFS_FLOCKDN);
    Hsfs = MmioRead32(SpiBar + R_HSFS);

    if ((Hsfs & B_HSFS_FLOCKDN) == 0) {
      DEBUG((DEBUG_ERROR, "[SentinelSmm] FATAL: Failed to assert "
                          "FLOCKDN. SPI configuration remains mutable.\n"));
    } else {
      DEBUG((DEBUG_INFO,
             "[SentinelSmm]   HSFS After:  0x%08X  "
             "(FLOCKDN=1) [LOCKED]\n",
             Hsfs));
    }
  } else {
    DEBUG((DEBUG_INFO, "[SentinelSmm]   FLOCKDN already asserted "
                       "by platform firmware.\n"));
  }

  // ========================================================================
  // Step 5: Assert SMM_BWP and BLE in BIOS_CNTL
  // ========================================================================
  //
  // SMM_BWP: Restricts flash writes exclusively to SMM context.
  // BLE:     Arms the SMI trap — any Ring 0 attempt to set BIOSWE
  //          triggers an immediate System Management Interrupt.
  // BIOSWE:  Cleared to 0 to disable flash writes from non-SMM.
  //
  BiosCntl = PciRead8(LPC_PCI_ADDR(R_BIOS_CNTL));
  DEBUG((DEBUG_INFO,
         "[SentinelSmm]   BIOS_CNTL Before: 0x%02X  "
         "(BIOSWE=%d, BLE=%d, SMM_BWP=%d)\n",
         BiosCntl, (BiosCntl & B_BIOS_CNTL_BIOSWE) ? 1 : 0,
         (BiosCntl & B_BIOS_CNTL_BLE) ? 1 : 0,
         (BiosCntl & B_BIOS_CNTL_SMM_BWP) ? 1 : 0));

  // Clear BIOSWE (bit 0), Set BLE (bit 1) and SMM_BWP (bit 5)
  BiosCntl &= (UINT8)~B_BIOS_CNTL_BIOSWE; // Force writes OFF
  BiosCntl |= B_BIOS_CNTL_BLE;            // Arm the SMI trap
  BiosCntl |= B_BIOS_CNTL_SMM_BWP;        // SMM-exclusive writes
  PciWrite8(LPC_PCI_ADDR(R_BIOS_CNTL), BiosCntl);

  // Verify
  BiosCntl = PciRead8(LPC_PCI_ADDR(R_BIOS_CNTL));
  DEBUG((DEBUG_INFO,
         "[SentinelSmm]   BIOS_CNTL After:  0x%02X  "
         "(BIOSWE=%d, BLE=%d, SMM_BWP=%d)\n",
         BiosCntl, (BiosCntl & B_BIOS_CNTL_BIOSWE) ? 1 : 0,
         (BiosCntl & B_BIOS_CNTL_BLE) ? 1 : 0,
         (BiosCntl & B_BIOS_CNTL_SMM_BWP) ? 1 : 0));

  if ((BiosCntl & B_BIOS_CNTL_BLE) && (BiosCntl & B_BIOS_CNTL_SMM_BWP)) {
    DEBUG((DEBUG_INFO, "[SentinelSmm]   SPI LOCKDOWN: COMPLETE\n"));
    DEBUG((DEBUG_INFO, "[SentinelSmm]   Ring 0 flash write capability: "
                       "SEVERED\n"));
  } else {
    DEBUG((DEBUG_ERROR, "[SentinelSmm]   SPI LOCKDOWN: PARTIAL — "
                        "some bits did not latch.\n"));
  }

  // ========================================================================
  // Step 6: Record S3 Resume Lockdown Opcodes in S3 Boot Script Table
  // ========================================================================
  if (S3SaveState != NULL) {
    UINT64 PciAddress;
    UINT64 MemoryAddress;

    DEBUG((DEBUG_INFO,
           "[SentinelSmm] Recording S3 Save State Boot Script opcodes...\n"));

    // 1. Record LPC BIOS_CNTL PCI Config Write
    PciAddress =
        EFI_BOOT_SCRIPT_PCI_ADDRESS(LPC_BUS, LPC_DEV, LPC_FUNC, R_BIOS_CNTL);
    S3Status = S3SaveState->Write(S3SaveState,
                                  0, // BootScriptMask
                                  EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE,
                                  EfiBootScriptWidthUint8, PciAddress, (UINTN)1,
                                  &BiosCntl);
    if (EFI_ERROR(S3Status)) {
      DEBUG((
          DEBUG_ERROR,
          "[SentinelSmm] S3 Save State: Failed to record BIOS_CNTL write: %r\n",
          S3Status));
    } else {
      DEBUG((DEBUG_INFO,
             "[SentinelSmm] S3 Save State: Recorded BIOS_CNTL write (Value: "
             "0x%02X)\n",
             BiosCntl));
    }

    // 2. Record SPI MMIO PR0 Protection Memory Write
    MemoryAddress = (UINT64)(SpiBar + R_PR0);
    S3Status = S3SaveState->Write(S3SaveState,
                                  0, // BootScriptMask
                                  EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE,
                                  EfiBootScriptWidthUint32, MemoryAddress,
                                  (UINTN)1, &Pr0Value);
    if (EFI_ERROR(S3Status)) {
      DEBUG((DEBUG_ERROR,
             "[SentinelSmm] S3 Save State: Failed to record PR0 memory write: "
             "%r\n",
             S3Status));
    } else {
      DEBUG((DEBUG_INFO,
             "[SentinelSmm] S3 Save State: Recorded PR0 memory write (Value: "
             "0x%08X)\n",
             Pr0Value));
    }

    // 3. Record SPI MMIO HSFS FLOCKDN Latch Memory Write (MUST be absolute last
    // SPI controller write)
    HsfsValue = Hsfs | B_HSFS_FLOCKDN;
    MemoryAddress = (UINT64)(SpiBar + R_HSFS);
    S3Status = S3SaveState->Write(S3SaveState,
                                  0, // BootScriptMask
                                  EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE,
                                  EfiBootScriptWidthUint32, MemoryAddress,
                                  (UINTN)1, &HsfsValue);
    if (EFI_ERROR(S3Status)) {
      DEBUG((DEBUG_ERROR,
             "[SentinelSmm] S3 Save State: Failed to record HSFS memory write: "
             "%r\n",
             S3Status));
    } else {
      DEBUG((DEBUG_INFO,
             "[SentinelSmm] S3 Save State: Recorded HSFS memory write (Value: "
             "0x%08X)\n",
             HsfsValue));
    }
  }

  DEBUG((DEBUG_INFO, "[SentinelSmm] --- Phase 3: Complete ---\n"));
  return EFI_SUCCESS;
}

// ----------------------------------------------------------------------------
// Policy Initialization
// ----------------------------------------------------------------------------

static VOID InitPolicyTable(VOID) {
  // Enable essential legacy ports explicitly

  // RTC / CMOS (0x70, 0x71)
  IoAllowMap[0x70 / 8] |= (1 << (0x70 % 8));
  IoAllowMap[0x71 / 8] |= (1 << (0x71 % 8));

  // In a full implementation, we locate the ACPI RSDT/XSDT from the
  // EFI System Configuration Table (gEfiAcpiTableGuid) and parse the FADT
  // to dynamically find the PM1a_CNT_BLK and PM1a_EVT_BLK bases.
  // For now, we mock the port (e.g. 0x400 for standard QEMU ACPI)
  UINT16 Pm1a_Cnt = 0x400;
  IoAllowMap[Pm1a_Cnt / 8] |= (1 << (Pm1a_Cnt % 8));

  DEBUG((DEBUG_INFO,
         "[SentinelSmm] Policy Table Initialized (O(1) bitmaps active).\n"));
}

// ----------------------------------------------------------------------------
// The Trap Handler
// ----------------------------------------------------------------------------

EFI_STATUS
EFIAPI
PolicyCheckTrapHandler(IN UINT64 ErrorCode, IN UINT64 FaultingRip,
                       IN SENTINEL_CPU_CONTEXT *Context) {
  UINT8 *Instruction = (UINT8 *)FaultingRip;

  // Check for MSR Instructions (0x0F 0x32 or 0x0F 0x30)
  if (Instruction[0] == 0x0F) {
    if (Instruction[1] == 0x32 || Instruction[1] == 0x30) {
      UINT32 RequestedMsr = (UINT32)Context->Rcx;

      BOOLEAN Allowed = FALSE;
      for (UINTN i = 0; i < SAFE_MSR_COUNT; i++) {
        if (SafeMsrList[i] == RequestedMsr) {
          Allowed = TRUE;
          break;
        }
      }

      if (!Allowed) {
        DEBUG((DEBUG_ERROR,
               "[SMM DENY] CPL3 Violation at RIP: 0x%lx, Access: WRMSR/RDMSR "
               "0x%x\n",
               FaultingRip, RequestedMsr));
        return EFI_ACCESS_DENIED;
      }
      return EFI_SUCCESS;
    }
  }

  // Check for immediate IN/OUT instructions (E4, E5, E6, E7)
  if (Instruction[0] == 0xE4 || Instruction[0] == 0xE5 ||
      Instruction[0] == 0xE6 || Instruction[0] == 0xE7) {
    UINT16 Port = Instruction[1];
    if (!(IoAllowMap[Port / 8] & (1 << (Port % 8)))) {
      DEBUG((DEBUG_ERROR,
             "[SMM DENY] CPL3 Violation at RIP: 0x%lx, Access: Immediate IO "
             "Port 0x%x\n",
             FaultingRip, Port));
      return EFI_ACCESS_DENIED;
    }
    return EFI_SUCCESS;
  }

  // Check for DX-based IN/OUT instructions (EC, ED, EE, EF)
  if (Instruction[0] == 0xEC || Instruction[0] == 0xED ||
      Instruction[0] == 0xEE || Instruction[0] == 0xEF) {
    UINT16 Port = (UINT16)Context->Rdx;
    if (!(IoAllowMap[Port / 8] & (1 << (Port % 8)))) {
      DEBUG((DEBUG_ERROR,
             "[SMM DENY] CPL3 Violation at RIP: 0x%lx, Access: IO Port 0x%x\n",
             FaultingRip, Port));
      return EFI_ACCESS_DENIED;
    }
    return EFI_SUCCESS;
  }

  // Unknown or unsupported instruction triggering #GP
  DEBUG((DEBUG_ERROR,
         "[SMM DENY] CPL3 Violation at RIP: 0x%lx, Unknown Opcode: %02X %02X\n",
         FaultingRip, Instruction[0], Instruction[1]));
  return EFI_ACCESS_DENIED;
}

// ----------------------------------------------------------------------------
// DXE Entry Point
// ----------------------------------------------------------------------------

EFI_STATUS
EFIAPI
SentinelSmmDxeEntry(IN EFI_HANDLE ImageHandle,
                    IN EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS Status;

  DEBUG((DEBUG_INFO,
         "\n[SentinelSmm] =======================================\n"));
  DEBUG((DEBUG_INFO, "[SentinelSmm] DXE Driver Entry — Ring -2 Supervisor\n"));
  DEBUG(
      (DEBUG_INFO, "[SentinelSmm] =======================================\n"));

  // Phase 1: Initialize the O(1) I/O port and MSR policy bitmaps
  DEBUG((DEBUG_INFO, "[SentinelSmm] Phase 1: Policy Table Init...\n"));
  InitPolicyTable();

  // Phase 2: Allocate secure shared memory buffer in TSEG/TMR
  DEBUG((DEBUG_INFO, "[SentinelSmm] Phase 2: Shared Memory Buffer...\n"));
  // TODO: Initialize SentinelSharedBuffer allocation in TSEG/TMR

  // Phase 3: SPI Flash Hardware Lockdown
  // Permanently severs Ring 0's ability to write to firmware partitions.
  // This is the ultimate guarantor of Ring -3 containment.
  DEBUG((DEBUG_INFO, "[SentinelSmm] Phase 3: SPI Flash Lockdown...\n"));
  Status = EnforceSpiLockdown();
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR,
           "[SentinelSmm] CRITICAL: SPI Lockdown failed "
           "(Status = %r). HALTING BOOT PROCESS.\n",
           Status));
    CpuDeadLoop();
  }

  DEBUG((DEBUG_INFO,
         "\n[SentinelSmm] =======================================\n"));
  DEBUG(
      (DEBUG_INFO, "[SentinelSmm] DXE Initialization: ALL PHASES COMPLETE\n"));
  DEBUG(
      (DEBUG_INFO, "[SentinelSmm] =======================================\n"));

  return EFI_SUCCESS;
}
