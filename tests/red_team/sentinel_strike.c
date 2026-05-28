#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <stdatomic.h>

// --- MEI Payload Definitions ---
typedef struct {
    uint8_t b[16];
} guid_t;

struct mei_client {
    uint32_t max_msg_length;
    uint8_t protocol_version;
    uint8_t reserved[3];
};

struct mei_connect_client_data {
    union {
        guid_t in_client_uuid;
        struct mei_client out_client_properties;
    };
};

// IOCTL command for MEI connect client
#define IOCTL_MEI_CONNECT_CLIENT _IOWR('H', 0x01, struct mei_connect_client_data)

// --- Global State ---
bool flood_mode = false;
atomic_int lsm_interdicts = 0;
atomic_int lsm_successes = 0;
atomic_int mei_interdicts = 0;
atomic_int mei_successes = 0;
atomic_int mei_missing = 0;
atomic_int xdp_packets_sent = 0;

// --- Thread 1: The Egress Violator (LSM) ---
void* attack_lsm(void* arg) {
    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_port = htons(443);
    inet_pton(AF_INET, "198.51.100.99", &target.sin_addr);

    do {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            usleep(100);
            continue;
        }

        int ret = connect(sock, (struct sockaddr*)&target, sizeof(target));
        if (ret < 0 && (errno == EPERM || errno == EACCES)) {
            atomic_fetch_add(&lsm_interdicts, 1);
        } else if (ret == 0 || errno == EINPROGRESS) {
            atomic_fetch_add(&lsm_successes, 1);
        }
        close(sock);
        
        if (flood_mode) usleep(500); // 0.5ms delay to prevent total fd exhaustion
    } while (flood_mode);
    
    return NULL;
}

// --- Thread 2: The Firmware Hijacker (SMM/HECI) ---
void* attack_mei(void* arg) {
    struct mei_connect_client_data data;
    // Intel AMT Remote Control GUID: {0x8E6A6715, 0x9ABC, 0x4043, {0x88, 0xEF, 0x9E, 0x38, 0xC6, 0x29, 0x4D, 0x07}}
    uint8_t amt_guid[16] = {
        0x15, 0x67, 0x6A, 0x8E, 0xBC, 0x9A, 0x43, 0x40,
        0x88, 0xEF, 0x9E, 0x38, 0xC6, 0x29, 0x4D, 0x07
    };
    memcpy(data.in_client_uuid.b, amt_guid, 16);

    do {
        int fd = open("/dev/mei0", O_RDWR);
        if (fd >= 0) {
            int ret = ioctl(fd, IOCTL_MEI_CONNECT_CLIENT, &data);
            if (ret < 0 && (errno == EPERM || errno == EACCES)) {
                atomic_fetch_add(&mei_interdicts, 1);
            } else if (ret == 0) {
                atomic_fetch_add(&mei_successes, 1);
            }
            close(fd);
        } else {
            if (errno == EPERM || errno == EACCES) {
                atomic_fetch_add(&mei_interdicts, 1);
            } else if (errno == ENOENT) {
                // Device not present, tally missing
                atomic_fetch_add(&mei_missing, 1);
                if (!flood_mode) break; // Don't spam if it's completely missing
                sleep(1); // Backoff
            }
        }
        
        if (flood_mode) usleep(1000); // 1ms delay
    } while (flood_mode);
    
    return NULL;
}

// --- Thread 3: The Ingress Flooder (XDP) ---
void* attack_xdp(void* arg) {
    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_port = htons(9999);
    inet_pton(AF_INET, "127.0.0.1", &target.sin_addr); // Loopback or dummy interface
    
    char payload[64] = "SENTINEL_XDP_FLOOD_PAYLOAD_TEST";

    do {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock >= 0) {
            sendto(sock, payload, sizeof(payload), 0, (struct sockaddr*)&target, sizeof(target));
            atomic_fetch_add(&xdp_packets_sent, 1);
            close(sock);
        }
        
        if (flood_mode) usleep(100); // 0.1ms delay
    } while (flood_mode);
    
    return NULL;
}

// --- Main Engine ---
int main(int argc, char** argv) {
    printf("==========================================\n");
    printf(" Sentinel Strike - Red Team Exploit Suite \n");
    printf("==========================================\n");

    // --- Execution Guardrail ---
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        if (strcmp(hostname, "intelhost") != 0 && strcmp(hostname, "raspberrypi") != 0) {
            fprintf(stderr, "[!] FATAL: Execution aborted to protect host machine.\n");
            fprintf(stderr, "[!] Hostname '%s' does not match target lab environment ('intelhost' or 'raspberrypi').\n", hostname);
            exit(1);
        }
    }

    if (argc > 1 && strcmp(argv[1], "--flood") == 0) {
        flood_mode = true;
        printf("[*] Mode: FLOOD (Infinite Loop, Max Concurrency)\n");
    } else {
        printf("[*] Mode: AUDIT (Single Run Validation)\n");
    }
    printf("\n");

    pthread_t t_lsm, t_mei, t_xdp;
    
    printf("[+] Spawning Thread 1: Egress Violator (LSM)...\n");
    pthread_create(&t_lsm, NULL, attack_lsm, NULL);
    
    printf("[+] Spawning Thread 2: Firmware Hijacker (SMM/HECI)...\n");
    pthread_create(&t_mei, NULL, attack_mei, NULL);
    
    printf("[+] Spawning Thread 3: Ingress Flooder (XDP)...\n");
    pthread_create(&t_xdp, NULL, attack_xdp, NULL);

    if (!flood_mode) {
        // Wait for threads to complete a single iteration
        pthread_join(t_lsm, NULL);
        pthread_join(t_mei, NULL);
        pthread_join(t_xdp, NULL);
        
        printf("\n--- AUDIT RESULTS ---\n");
        printf("LSM Interdicts : %d (Successes: %d)\n", lsm_interdicts, lsm_successes);
        
        if (mei_missing > 0) {
            printf("MEI Interdicts : N/A (/dev/mei0 not found on this node)\n");
        } else {
            printf("MEI Interdicts : %d (Successes: %d)\n", mei_interdicts, mei_successes);
        }
        
        printf("XDP TX Packets : %d (Check XDP hook telemetry for drop verification)\n", xdp_packets_sent);
        printf("---------------------\n");
    } else {
        // Infinite telemetry loop
        while (1) {
            sleep(2);
            printf("[TELEMETRY] LSM Drops: %6d | MEI Drops: %6d | XDP Sent: %6d\n",
                lsm_interdicts, mei_interdicts, xdp_packets_sent);
        }
    }
    
    return 0;
}
