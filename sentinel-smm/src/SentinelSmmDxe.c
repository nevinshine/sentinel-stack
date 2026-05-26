#include <PiDxe.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include "SentinelSharedBuffer.h"
#include "SentinelSmmCpuContext.h"

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

// SPI MMIO Range
#define SPI_BASE_ADDR  0xFED01000
#define SPI_LIMIT_ADDR 0xFED02000

// ----------------------------------------------------------------------------
// Policy Initialization
// ----------------------------------------------------------------------------

static VOID InitPolicyTable(VOID)
{
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
    
    DEBUG ((DEBUG_INFO, "[SentinelSmm] Policy Table Initialized (O(1) bitmaps active).\n"));
}

// ----------------------------------------------------------------------------
// The Trap Handler
// ----------------------------------------------------------------------------

EFI_STATUS
EFIAPI
PolicyCheckTrapHandler(
    IN UINT64                ErrorCode,
    IN UINT64                FaultingRip,
    IN SENTINEL_CPU_CONTEXT  *Context
    )
{
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
                DEBUG ((DEBUG_ERROR, "[SMM DENY] CPL3 Violation at RIP: 0x%lx, Access: WRMSR/RDMSR 0x%x\n", FaultingRip, RequestedMsr));
                return EFI_ACCESS_DENIED;
            }
            return EFI_SUCCESS;
        }
    }
    
    // Check for immediate IN/OUT instructions (E4, E5, E6, E7)
    if (Instruction[0] == 0xE4 || Instruction[0] == 0xE5 || Instruction[0] == 0xE6 || Instruction[0] == 0xE7) {
        UINT16 Port = Instruction[1];
        if (!(IoAllowMap[Port / 8] & (1 << (Port % 8)))) {
            DEBUG ((DEBUG_ERROR, "[SMM DENY] CPL3 Violation at RIP: 0x%lx, Access: Immediate IO Port 0x%x\n", FaultingRip, Port));
            return EFI_ACCESS_DENIED;
        }
        return EFI_SUCCESS;
    }
    
    // Check for DX-based IN/OUT instructions (EC, ED, EE, EF)
    if (Instruction[0] == 0xEC || Instruction[0] == 0xED || Instruction[0] == 0xEE || Instruction[0] == 0xEF) {
        UINT16 Port = (UINT16)Context->Rdx;
        if (!(IoAllowMap[Port / 8] & (1 << (Port % 8)))) {
            DEBUG ((DEBUG_ERROR, "[SMM DENY] CPL3 Violation at RIP: 0x%lx, Access: IO Port 0x%x\n", FaultingRip, Port));
            return EFI_ACCESS_DENIED;
        }
        return EFI_SUCCESS;
    }
    
    // Unknown or unsupported instruction triggering #GP
    DEBUG ((DEBUG_ERROR, "[SMM DENY] CPL3 Violation at RIP: 0x%lx, Unknown Opcode: %02X %02X\n", FaultingRip, Instruction[0], Instruction[1]));
    return EFI_ACCESS_DENIED;
}

// ----------------------------------------------------------------------------
// DXE Entry Point
// ----------------------------------------------------------------------------

EFI_STATUS
EFIAPI
SentinelSmmDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  DEBUG ((DEBUG_INFO, "\n[SentinelSmm] =======================================\n"));
  DEBUG ((DEBUG_INFO, "[SentinelSmm] DXE Driver Entry Phase 1 Init...\n"));
  
  InitPolicyTable();
  
  DEBUG ((DEBUG_INFO, "[SentinelSmm] Allocating Circular Shared Memory Buffer...\n"));
  // TODO: Initialize SentinelSharedBuffer allocation in TSEG/TMR
  
  DEBUG ((DEBUG_INFO, "[SentinelSmm] DXE Driver Initialization Complete.\n"));
  DEBUG ((DEBUG_INFO, "[SentinelSmm] =======================================\n"));
  
  return EFI_SUCCESS;
}
