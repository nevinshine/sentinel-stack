/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * mock_mei_attack.c — Live-Fire HECI Interdiction Test Payload
 *
 * This utility simulates an adversarial attack or unauthorized Ring 0 utility
 * attempting to interact with restricted Intel Management Engine services via
 * the Host Embedded Controller Interface (HECI).
 *
 * It attempts to:
 *   1. Open the HECI character device (/dev/mei0 or user-provided device path).
 *   2. Connect to restricted ME Client GUIDs (e.g. AMT SOL, AMT Remote Control,
 *      ICC Clock Control) or the non-restricted MKHI interface.
 *   3. Transmit a payload to verify if the Kretprobe in `sentinel_mei_hook.ko`
 *      intercepts the transaction and returns -EPERM.
 *
 * Copyright (C) 2026 Nevin Shine <nevinshine05@outlook.com>
 */

#include <errno.h>
#include <fcntl.h>
#include <linux/mei.h>
#include <linux/uuid.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* Restricted Intel ME Client GUIDs matching sentinel_mei_hook.h */
static const uuid_le AMT_REMOTE_CONTROL_GUID = UUID_LE(
    0x12F80028, 0xB4B7, 0x4B2D, 0xAC, 0xA8, 0x46, 0xE0, 0xFF, 0x65, 0x81, 0x4C);

static const uuid_le AMT_SOL_GUID = UUID_LE(
    0xFB3B192E, 0xE714, 0x44A4, 0x8B, 0x22, 0x55, 0x82, 0x23, 0xDE, 0x6F, 0xF4);

static const uuid_le ICC_CLOCK_CONTROL_GUID = UUID_LE(
    0xF934D0F2, 0x3E42, 0x402E, 0x8C, 0xDF, 0x73, 0x01, 0x02, 0xAA, 0x13, 0xD1);

static const uuid_le MKHI_GUID = UUID_LE(0x8E6A6715, 0x9ABC, 0x4043, 0x88, 0xEF,
                                         0x9E, 0x39, 0xC6, 0xF6, 0x3E, 0x0F);

struct target_client {
  const char *name;
  const uuid_le *uuid;
  int is_restricted;
};

static const struct target_client target_clients[] = {
    {"AMT Remote Control", &AMT_REMOTE_CONTROL_GUID, 1},
    {"AMT Serial-Over-LAN", &AMT_SOL_GUID, 1},
    {"ICC Clock Control", &ICC_CLOCK_CONTROL_GUID, 1},
    {"MKHI Management", &MKHI_GUID, 0},
};
#define TARGET_CLIENTS_COUNT                                                   \
  (sizeof(target_clients) / sizeof(struct target_client))

static void usage(const char *progname) {
  fprintf(stderr, "Usage: %s [OPTIONS]\n", progname);
  fprintf(stderr,
          "Sentinel Live-Fire HECI Interdiction Validation Utility\n\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -d, --device <path>   Use custom HECI character device "
                  "(default: /dev/mei0)\n");
  fprintf(stderr, "  -h, --help            Display this help menu\n");
}

int main(int argc, char *argv[]) {
  const char *device_path = "/dev/mei0";
  int fd;
  int i;
  int ret;

  printf("=== Sentinel Ring -3 Live-Fire Interdiction Test ===\n");

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--device") == 0) {
      if (i + 1 < argc) {
        device_path = argv[++i];
      } else {
        fprintf(stderr, "Error: Option %s requires a path\n", argv[i]);
        usage(argv[0]);
        return EXIT_FAILURE;
      }
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      usage(argv[0]);
      return EXIT_SUCCESS;
    } else {
      fprintf(stderr, "Error: Unknown argument '%s'\n", argv[i]);
      usage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  printf("[*] Target Device: %s\n", device_path);

  for (i = 0; i < (int)TARGET_CLIENTS_COUNT; i++) {
    const struct target_client *client = &target_clients[i];
    struct mei_connect_client_data conn_data;
    char uuid_str[64];

    printf("\n------------------------------------------------------------\n");
    printf("[*] Client: %s (%s)\n", client->name,
           client->is_restricted ? "RESTRICTED - Should be Blocked"
                                 : "NON-RESTRICTED - Should Pass");

    // Format UUID string for output using individual byte elements of uuid_le
    snprintf(
        uuid_str, sizeof(uuid_str),
        "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        client->uuid->b[3], client->uuid->b[2], client->uuid->b[1],
        client->uuid->b[0], client->uuid->b[5], client->uuid->b[4],
        client->uuid->b[7], client->uuid->b[6], client->uuid->b[8],
        client->uuid->b[9], client->uuid->b[10], client->uuid->b[11],
        client->uuid->b[12], client->uuid->b[13], client->uuid->b[14],
        client->uuid->b[15]);
    printf("[*] Target GUID: %s\n", uuid_str);

    // 1. Open HECI interface
    fd = open(device_path, O_RDWR);
    if (fd < 0) {
      printf("[!] Open failed: %s (Check if module loaded or path exists)\n",
             strerror(errno));
      continue;
    }

    // 2. Attempt Connection
    memset(&conn_data, 0, sizeof(conn_data));
    memcpy(&conn_data.in_client_uuid, client->uuid, sizeof(uuid_le));

    printf("[*] Attempting MEI_CONNECT_CLIENT_IOCTL...\n");
    ret = ioctl(fd, IOCTL_MEI_CONNECT_CLIENT, &conn_data);
    if (ret < 0) {
      int err = errno;
      printf("[!] IOCTL failed: %s (errno: %d)\n", strerror(err), err);
      if (err == EPERM) {
        printf("[SUCCESS] EPERM received! sentinel_mei_hook blocked the "
               "connection.\n");
      }
      close(fd);
      continue;
    }

    printf("[+] Connection succeeded. Max message length: %d\n",
           conn_data.out_client_properties.max_msg_length);

    // 3. Attempt write to trigger mei_cldev_send
    uint8_t payload[8] = {0xAA, 0x55, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    printf("[*] Attempting write (triggering HECI send)...\n");
    ssize_t written = write(fd, payload, sizeof(payload));
    if (written < 0) {
      int err = errno;
      printf("[!] Write failed: %s (errno: %d)\n", strerror(err), err);
      if (err == EPERM) {
        printf("[SUCCESS] EPERM received on HECI send! sentinel_mei_hook "
               "successfully interdicted write.\n");
      }
    } else {
      printf("[PASS] Write succeeded: %zd bytes sent.\n", written);
    }

    close(fd);
  }

  printf("\n============================================================\n");
  printf("Live-Fire Test Run Finished.\n");
  return EXIT_SUCCESS;
}
