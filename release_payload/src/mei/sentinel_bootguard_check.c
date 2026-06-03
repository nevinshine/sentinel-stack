/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * sentinel_bootguard_check.c — Boot Guard Pre-flight Check
 *
 * Copyright (C) 2026 Nevin Shine <nevinshine05@outlook.com>
 */

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "sentinel_bootguard_check.h"

bool sentinel_bootguard_is_verified_boot_active(void) {
  int fd;
  uint64_t msr_val = 0;
  ssize_t bytes_read;

  /* Require root to read MSRs */
  if (geteuid() != 0) {
    fprintf(stderr,
            "[BootGuard] Error: Root privileges required to read MSRs.\n");
    return false; /* Fail open? No, the caller should handle the error. But for
                     safety, return true to block? Actually let's return true on
                     error to be safe. */
  }

  fd = open("/dev/cpu/0/msr", O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "[BootGuard] Warning: Could not open /dev/cpu/0/msr.\n");
    fprintf(stderr, "[BootGuard] Make sure the 'msr' kernel module is loaded "
                    "(modprobe msr).\n");
    return true; /* Block if we can't verify */
  }

  /* Read MSR 0x13A */
  if (lseek(fd, MSR_BOOT_GUARD_SACM_INFO, SEEK_SET) < 0) {
    perror("[BootGuard] lseek MSR 0x13A failed");
    close(fd);
    return true; /* Block on error */
  }

  bytes_read = read(fd, &msr_val, sizeof(msr_val));
  close(fd);

  if (bytes_read != sizeof(msr_val)) {
    perror("[BootGuard] Failed to read MSR 0x13A");
    return true; /* Block on error */
  }

  printf("[BootGuard] MSR 0x13A (SACM_INFO) = 0x%016lx\n", msr_val);

  if (!(msr_val & B_BOOT_GUARD_NEM_ENABLED)) {
    printf("[BootGuard] Boot Guard capability is not present on this CPU.\n");
    return false;
  }

  if (!(msr_val & B_BOOT_GUARD_ENABLE)) {
    printf("[BootGuard] Boot Guard is disabled.\n");
    return false;
  }

  if (msr_val & B_BOOT_GUARD_VERIFIED_BOOT) {
    printf("[BootGuard] CRITICAL: Verified Boot (Profile 4/5) is ACTIVE.\n");
    printf("[BootGuard] The ME provides the cryptographic root of trust for "
           "this platform.\n");
    printf("[BootGuard] Disabling the ME will result in a hardware BRICK on "
           "next reboot.\n");
    return true; /* VERIFIED BOOT IS ACTIVE -> DO NOT DISABLE ME */
  }

  if (msr_val & B_BOOT_GUARD_MEASURED_BOOT) {
    printf("[BootGuard] Measured Boot is active, but Verified Boot is OFF.\n");
    printf("[BootGuard] ME disable is potentially safe, though PCR registers "
           "will change.\n");
    return false; /* Safe to disable */
  }

  return false;
}
