/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * sentinel_me_disable.c — HECI Soft-Disable Enforcer
 *
 * This utility connects to the Intel Management Engine via the MKHI
 * (ME Kernel Host Interface) client and transmits a SET_ME_DISABLE
 * command. This forces the ME to gracefully transition into a dormant
 * state (often referred to as "Soft-Disable" or HAP mode equivalent).
 *
 * WARNING: This operation is highly destructive to the ME state machine
 * and can result in a hardware brick if Intel Boot Guard Profile 4 or 5
 * (Verified Boot) is active. It is gated behind a pre-flight Boot Guard
 * check and a strict command-line flag.
 *
 * Copyright (C) 2026 Nevin Shine <nevinshine05@outlook.com>
 */

#include <errno.h>
#include <fcntl.h>
#include <linux/mei.h>
#include <linux/uuid.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "sentinel_bootguard_check.h"

/* MKHI Client GUID: 8E6A6715-9ABC-4043-88EF-9E39C6F63E0F */
static const uuid_le MKHI_GUID = UUID_LE(0x8E6A6715, 0x9ABC, 0x4043, 0x88, 0xEF,
                                         0x9E, 0x39, 0xC6, 0xF6, 0x3E, 0x0F);

/*
 * MKHI Message Header
 * Group ID: 0x03 (GEN - General)
 * Command:  0x03 (FW_STS) is common, but HMRFPO is Group 0x05, Cmd 0x01.
 * Actually, the standard ME disable via MKHI is often achieved using
 * the HMRFPO (Host ME Region Flash Protection Override) ENABLE message,
 * or the proprietary SET_ME_DISABLE command on newer firmwares.
 * For this implementation, we use the universally recognized GEN Group (0x03)
 * SET_ME_DISABLE command (Cmd 0x05) used by coreboot / me_cleaner soft-disable.
 */
struct mkhi_header {
  uint32_t group_id : 8;
  uint32_t command : 7;
  uint32_t is_response : 1;
  uint32_t reserved : 8;
  uint32_t result : 8;
} __attribute__((packed));

struct mkhi_msg_me_disable {
  struct mkhi_header hdr;
  /* No payload required for this command */
} __attribute__((packed));

struct mkhi_msg_me_disable_resp {
  struct mkhi_header hdr;
} __attribute__((packed));

#define MKHI_GROUP_GEN 0x03
#define MKHI_GEN_CMD_ME_DISABLE 0x05

static void usage(const char *progname) {
  fprintf(stderr, "Usage: %s [OPTIONS]\n", progname);
  fprintf(stderr, "Sentinel Ring -3 HECI Soft-Disable Enforcer\n\n");
  fprintf(stderr, "  --enable-me-disable    Acknowledge the risks and execute "
                  "the soft-disable\n");
  fprintf(stderr, "  -h, --help             Display this help message\n");
}

int main(int argc, char *argv[]) {
  int fd;
  int ret;
  struct mei_connect_client_data data;
  struct mkhi_msg_me_disable req;
  struct mkhi_msg_me_disable_resp resp;
  ssize_t bytes;
  bool force_disable = false;

  printf("=== Sentinel Ring -3 HECI Soft-Disable Enforcer ===\n");

  if (argc < 2) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "--enable-me-disable") == 0) {
    force_disable = true;
  } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
    usage(argv[0]);
    return EXIT_SUCCESS;
  } else {
    fprintf(stderr, "Error: Unknown argument '%s'\n", argv[1]);
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (geteuid() != 0) {
    fprintf(stderr, "Error: Must run as root to access /dev/mei0 and MSRs.\n");
    return EXIT_FAILURE;
  }

  /* ---------------------------------------------------------
   * Pre-flight Check: Intel Boot Guard
   * --------------------------------------------------------- */
  printf("\n[Phase 1] Executing Boot Guard Pre-flight Check...\n");
  if (sentinel_bootguard_is_verified_boot_active()) {
    fprintf(stderr, "\n[ABORT] Boot Guard is enforcing the boot chain.\n");
    fprintf(stderr, "[ABORT] Soft-disabling the ME will BRICK this system.\n");
    return EXIT_FAILURE;
  }
  printf("[Phase 1] Pass: Boot Guard verified boot is not active.\n");

  if (!force_disable) {
    /* We shouldn't hit this due to arg parsing, but defensive */
    return EXIT_FAILURE;
  }

  /* ---------------------------------------------------------
   * Phase 2: Connect to MKHI
   * --------------------------------------------------------- */
  printf("\n[Phase 2] Opening HECI interface (/dev/mei0)...\n");
  fd = open("/dev/mei0", O_RDWR);
  if (fd < 0) {
    perror("open /dev/mei0 failed");
    fprintf(stderr, "Ensure the 'mei_me' module is loaded.\n");
    return EXIT_FAILURE;
  }

  memset(&data, 0, sizeof(data));
  memcpy(&data.in_client_uuid, &MKHI_GUID, sizeof(uuid_le));

  printf("[Phase 2] Connecting to MKHI Client...\n");
  ret = ioctl(fd, IOCTL_MEI_CONNECT_CLIENT, &data);
  if (ret < 0) {
    perror("ioctl IOCTL_MEI_CONNECT_CLIENT failed");
    close(fd);
    return EXIT_FAILURE;
  }
  printf("[Phase 2] Connected. Max message length: %d bytes\n",
         data.out_client_properties.max_msg_length);

  /* ---------------------------------------------------------
   * Phase 3: Transmit SET_ME_DISABLE
   * --------------------------------------------------------- */
  printf("\n[Phase 3] Constructing SET_ME_DISABLE command...\n");
  memset(&req, 0, sizeof(req));
  req.hdr.group_id = MKHI_GROUP_GEN;
  req.hdr.command = MKHI_GEN_CMD_ME_DISABLE;
  req.hdr.is_response = 0;

  printf("[Phase 3] Transmitting command to Ring -3...\n");
  bytes = write(fd, &req, sizeof(req));
  if (bytes < 0) {
    perror("write to MKHI failed");
    /* Check if our own sentinel_mei_hook blocked it */
    if (errno == EPERM) {
      fprintf(stderr, "Error: Command blocked (EPERM). Is sentinel_mei_hook "
                      "loaded with strict_mkhi=1?\n");
    }
    close(fd);
    return EXIT_FAILURE;
  }

  /* Read Response */
  printf("[Phase 3] Awaiting ME response...\n");
  bytes = read(fd, &resp, sizeof(resp));
  if (bytes < 0) {
    perror("read from MKHI failed");
    close(fd);
    return EXIT_FAILURE;
  }

  if (bytes < (ssize_t)sizeof(struct mkhi_header)) {
    fprintf(stderr, "Error: Invalid response size (%zd bytes)\n", bytes);
    close(fd);
    return EXIT_FAILURE;
  }

  if (resp.hdr.result == 0) {
    printf("[SUCCESS] ME acknowledged the soft-disable command.\n");
    printf("[SUCCESS] The ME will enter a dormant state after the next global "
           "reset.\n");
  } else {
    printf("[FAILURE] ME rejected the command. Result code: 0x%02X\n",
           resp.hdr.result);
  }

  close(fd);
  printf("\n=== Execution Complete ===\n");
  return EXIT_SUCCESS;
}
