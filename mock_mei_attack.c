#include <errno.h>
#include <fcntl.h>
#include <linux/mei.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

// Intel AMT Remote Control GUID (Targeting the restricted client)
const unsigned char amt_guid[16] = {0x28, 0x00, 0xF8, 0x12, 0xB7, 0xB4,
                                    0x2D, 0x4B, 0xAC, 0xA8, 0x46, 0xE0,
                                    0xFF, 0x65, 0x81, 0x4C};

int main() {
  struct mei_connect_client_data data;
  printf("[*] Firing IOCTL_MEI_CONNECT_CLIENT for AMT Remote Control...\n");

  int fd = open("/dev/mei0", O_RDWR);
  if (fd < 0)
    return EXIT_FAILURE;

  memset(&data, 0, sizeof(data));
  memcpy(&data.in_client_uuid, amt_guid, 16);

  if (ioctl(fd, IOCTL_MEI_CONNECT_CLIENT, &data) < 0 && errno == EPERM) {
    printf("[+] SUCCESS: Sentinel Kprobe intercepted and blocked (EPERM)!\n");
  } else {
    printf("[-] FAILURE: Interdiction bypassed.\n");
  }
  return 0;
}
