/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * sentinel_bootguard_check.h — Boot Guard Pre-flight Check
 *
 * Verifies Intel Boot Guard status via MSR 0x13A before allowing
 * high-risk ME operations like HECI Soft-Disable.
 *
 * Copyright (C) 2026 Nevin Shine <nevinshine05@outlook.com>
 */

#ifndef _SENTINEL_BOOTGUARD_CHECK_H
#define _SENTINEL_BOOTGUARD_CHECK_H

#include <stdbool.h>
#include <stdint.h>

/*
 * MSR_BOOT_GUARD_SACM_INFO (0x13A)
 * Bit 0: NemEnabled (Boot Guard Capability)
 * Bit 1: Measured Boot Enable
 * Bit 2: Verified Boot Enable
 * Bit 3: Revocation Enable
 * Bit 4: Boot Guard Enable
 * Bits 32:63: TXT Status
 */
#define MSR_BOOT_GUARD_SACM_INFO 0x13A

#define B_BOOT_GUARD_NEM_ENABLED (1ULL << 0)
#define B_BOOT_GUARD_MEASURED_BOOT (1ULL << 1)
#define B_BOOT_GUARD_VERIFIED_BOOT (1ULL << 2)
#define B_BOOT_GUARD_ENABLE (1ULL << 4)

/*
 * Checks if Boot Guard Verified Boot is actively enforcing the ME/BIOS
 * boot chain.
 *
 * Returns:
 *   true  - Boot Guard is ACTIVE and VERIFIED BOOT is enabled.
 *           (DO NOT DISABLE ME)
 *   false - Boot Guard is DISABLED, INACTIVE, or MEASURED-ONLY.
 *           (SAFE TO DISABLE ME)
 */
bool sentinel_bootguard_is_verified_boot_active(void);

#endif /* _SENTINEL_BOOTGUARD_CHECK_H */
