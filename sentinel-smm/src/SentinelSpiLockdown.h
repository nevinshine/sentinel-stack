/** @file
  SentinelSpiLockdown.h — ICH9/Q35 SPI Flash Protection Register Definitions

  Defines the PCI Configuration Space and Memory-Mapped I/O register
  constants required to enforce hardware-level SPI flash write protection
  on Intel ICH9 (QEMU q35) and compatible Platform Controller Hubs.

  Register Map Source: Intel I/O Controller Hub 9 (ICH9) Datasheet,
  Document Number 316972-004, Sections 10.1, 10.3, 21.1, 21.17.

  Copyright (C) 2026 Nevin Shine <nevinshine05@outlook.com>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _SENTINEL_SPI_LOCKDOWN_H
#define _SENTINEL_SPI_LOCKDOWN_H

#include <Library/PciLib.h>
#include <Library/IoLib.h>

// ============================================================================
// LPC/eSPI Controller — PCI Configuration Space
// ============================================================================
//
// The LPC Interface Bridge resides at Bus 0, Device 31, Function 0.
// This is the primary configuration endpoint for chipset-level flash
// protection registers on ICH9 and all modern Intel PCH variants.
//

#define LPC_BUS      0
#define LPC_DEV      31
#define LPC_FUNC     0

//
// Macro to construct a PCI_LIB_ADDRESS for the LPC controller.
// Usage: PciRead32(LPC_PCI_ADDR(Offset))
//
#define LPC_PCI_ADDR(Offset)  PCI_LIB_ADDRESS(LPC_BUS, LPC_DEV, LPC_FUNC, (Offset))

// ============================================================================
// BIOS_CNTL Register (PCI Offset 0xDC)
// ============================================================================
//
// The BIOS Control Register governs fundamental write-access policies
// for the SPI flash BIOS region. It is the first line of defense against
// unauthorized firmware modification.
//
//   Bit 0: BIOSWE  — BIOS Write Enable.
//                     0 = Flash writes are blocked by hardware.
//                     1 = Flash writes are permitted.
//                     This bit is the target of Ring 0 rootkits.
//
//   Bit 1: BLE     — BIOS Lock Enable. (Write-Once)
//                     When set to 1, any software attempt to change
//                     BIOSWE from 0→1 triggers an immediate System
//                     Management Interrupt (SMI). This transfers
//                     control to the SMM handler, which can forcibly
//                     clear BIOSWE before the write executes.
//                     Once set, BLE cannot be cleared until hardware reset.
//
//   Bit 5: SMM_BWP — SMM BIOS Write Protect.
//                     When set to 1, the SPI flash BIOS region becomes
//                     write-protected from ALL execution contexts UNLESS
//                     all logical processors are concurrently in SMM.
//                     This blocks even the most sophisticated Ring 0
//                     timing attacks and race conditions.
//

#define R_BIOS_CNTL         0xDC

#define B_BIOS_CNTL_BIOSWE  BIT0
#define B_BIOS_CNTL_BLE     BIT1
#define B_BIOS_CNTL_SMM_BWP BIT5

// ============================================================================
// RCBA — Root Complex Base Address Register (PCI Offset 0xF0)
// ============================================================================
//
// The RCBA provides the 32-bit base address for all chipset Memory-Mapped
// I/O (MMIO) registers, including the SPI controller configuration space.
//
//   Bits 31:14 — Base Address (16KB aligned)
//   Bit 0      — Enable bit (must be 1 for MMIO to be active)
//
// The SPIBAR (SPI Base Address Register) is located at RCBA + 0x3800.
//

#define R_RCBA              0xF0

#define B_RCBA_ENABLE       BIT0
#define RCBA_ADDR_MASK      0xFFFFC000U  // Bits 31:14

// ============================================================================
// SPIBAR — SPI Controller MMIO Register Block
// ============================================================================
//
// All SPI controller registers reside at a fixed offset from the RCBA.
// On ICH9, the SPI register block starts at RCBA + 0x3800.
//

#define SPIBAR_OFFSET       0x3800

// ============================================================================
// HSFS — Hardware Sequencing Flash Status (SPIBAR + 0x04)
// ============================================================================
//
// The HSFS register contains the FLOCKDN bit, the most critical
// configuration lockdown mechanism for the entire SPI subsystem.
//
//   Bit 15: FLOCKDN — Flash Configuration Lock-Down.
//                      When set to 1, the following registers become
//                      permanently read-only until hardware reset:
//                        - PR0-PR4 (Protected Range Registers)
//                        - FRAP (Flash Regions Access Permissions)
//                        - PREOP (Prefix Opcode Configuration)
//                        - SSFC (Software Sequencing Flash Control)
//
//                      WARNING: FLOCKDN is unlatched during S3 resume.
//                      The Sentinel SMM must hook S3 resume pathways
//                      to re-assert FLOCKDN before OS control resumes.
//                      (Deferred to a future sprint per design review.)
//

#define R_HSFS              0x04

#define B_HSFS_FLOCKDN      BIT15

// ============================================================================
// PR0-PR4 — Protected Range Registers (SPIBAR + 0x74 through 0x84)
// ============================================================================
//
// The Protected Range Registers define physical address windows on the
// SPI flash chip that are hardware-protected against writes. These
// registers override ALL software permissions, including SMM.
//
//   Bits 12:0   — Protected Range Base (in 4KB granularity units)
//   Bit  15     — Write Protection Enable (WP)
//   Bits 28:16  — Protected Range Limit (in 4KB granularity units)
//   Bit  31     — Read Protection Enable (RP)
//
// Physical address translation:
//   Start = Base * 4096
//   End   = (Limit + 1) * 4096 - 1
//
// When WP is set and FLOCKDN is asserted, the SPI hardware controller
// physically rejects all write operations targeting the defined range,
// even if the CPU is executing in SMM. This provides the ultimate
// hardware-backed guarantor of firmware integrity.
//

#define R_PR0               0x74
#define R_PR1               0x78
#define R_PR2               0x7C
#define R_PR3               0x80
#define R_PR4               0x84

#define B_PR_WP_ENABLE      BIT15
#define B_PR_RP_ENABLE      BIT31

//
// Macros to construct a PR register value from physical address bounds.
//
// Usage: PR_VALUE(0x00600000, 0x007FFFFF, TRUE, FALSE)
//   -> Protects SPI flash from 6MB to 8MB with write protection enabled.
//
#define PR_BASE(PhysAddr)   (((PhysAddr) >> 12) & 0x1FFF)
#define PR_LIMIT(PhysAddr)  ((((PhysAddr) >> 12) & 0x1FFF) << 16)
#define PR_VALUE(Base, Limit, WriteProtect, ReadProtect) \
    (PR_BASE(Base) | PR_LIMIT(Limit) | \
     ((WriteProtect) ? B_PR_WP_ENABLE : 0) | \
     ((ReadProtect) ? B_PR_RP_ENABLE : 0))

// ============================================================================
// Flash Descriptor — ME Region Boundaries
// ============================================================================
//
// The Intel Flash Descriptor (IFD) resides at the very start of the SPI
// flash (offset 0x00). The Flash Region registers (FREG0-FREG5) within
// the SPI MMIO space define the physical boundaries of each flash
// partition (Descriptor, BIOS, ME, GbE, Platform Data).
//
// FREG2 (Flash Region 2 — Intel ME):
//   Located at SPIBAR + 0x58
//   Bits 12:0   — Region Base (4KB units)
//   Bits 28:16  — Region Limit (4KB units)
//

#define R_FREG2_ME          0x58

#define FREG_BASE_MASK      0x00001FFFU
#define FREG_LIMIT_MASK     0x1FFF0000U
#define FREG_LIMIT_SHIFT    16

#endif /* _SENTINEL_SPI_LOCKDOWN_H */
