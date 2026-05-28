#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#define TARGET_IP "198.51.100.99"
#define TARGET_PORT 80

int my_atoi(const char *str) {
    int res = 0;
    for (int i = 0; str[i] != '\0'; ++i) {
        if (str[i] >= '0' && str[i] <= '9') {
            res = res * 10 + str[i] - '0';
        }
    }
    return res;
}

int flood_count = 10000;

// --- THREAD 1: LSM Syscall Violator ---
// Attempts to establish outbound TCP connections to a restricted IP.
// Expectation: Sentinel's BPF_LSM hook intercepts connect() and returns -EPERM.
void *lsm_egress_violator(void *arg) {
    int blocked_count = 0;
    struct sockaddr_in target;
    
    target.sin_family = AF_INET;
    target.sin_port = htons(TARGET_PORT);
    inet_pton(AF_INET, TARGET_IP, &target.sin_addr);

    printf("[*] LSM Thread: Firing %d outbound connect() attempts to %s...\n", flood_count, TARGET_IP);

    for (int i = 0; i < flood_count; i++) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(sock, (struct sockaddr *)&target, sizeof(target)) < 0) {
            if (errno == EPERM) {
                blocked_count++;
            }
        }
        close(sock);
    }
    
    printf("[+] LSM Thread: Successfully intercepted %d/%d connections (-EPERM).\n", blocked_count, flood_count);
    return NULL;
}

// --- THREAD 2: XDP Ingress Flooder ---
// Generates a massive burst of raw UDP packets to stress the XDP drop logic and ring buffer.
// Expectation: XDP silently drops packets at wire-speed before they hit the Linux network stack.
void *xdp_ingress_flooder(void *arg) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in target;
    char *payload = "SENTINEL_XDP_STRESS_TEST";
    
    target.sin_family = AF_INET;
    target.sin_port = htons(TARGET_PORT);
    inet_pton(AF_INET, TARGET_IP, &target.sin_addr);

    printf("[*] XDP Thread: Flooding %d UDP packets to %s...\n", flood_count, TARGET_IP);

    for (int i = 0; i < flood_count; i++) {
        sendto(sock, payload, strlen(payload), 0, (struct sockaddr *)&target, sizeof(target));
    }
    
    close(sock);
    printf("[+] XDP Thread: Flood complete. Awaiting Prometheus telemetry verification.\n");
    return NULL;
}

#include <sys/un.h>

void self_taint() {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) return;
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/var/run/telos.sock");
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        char msg[256];
        sprintf(msg, "{\"command\": \"UPDATE_TAINT\", \"data\": {\"pid\": %d, \"taint_level\": 4}}\n", getpid());
        send(sock, msg, strlen(msg), 0);
        printf("[+] Registered process %d as TAINT_CRITICAL via IPC.\n", getpid());
        sleep(1); // FIX: Wait for Daemon to update the BPF map!
    }
    close(sock);
}

int main(int argc, char *argv[]) {
    pthread_t lsm_thread, xdp_thread;

    if (argc > 1) {
        flood_count = my_atoi(argv[1]);
        if (flood_count <= 0) {
            flood_count = 10000;
        }
    }

    printf("==========================================\n");
    printf("   SENTINEL STRIKE: RING 0 PROD TEST      \n");
    printf("==========================================\n");

    // Taint this process so the LSM hook filters it
    self_taint();

    // --- Execution Guardrail ---
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        if (strcmp(hostname, "intelhost") != 0 && strcmp(hostname, "raspberrypi") != 0) {
            fprintf(stderr, "[!] FATAL: Execution aborted to protect host machine.\n");
            fprintf(stderr, "[!] Hostname '%s' does not match target lab environment ('intelhost' or 'raspberrypi').\n", hostname);
            exit(1);
        }
    }

    // Launch concurrent attacks against Ring 0
    pthread_create(&lsm_thread, NULL, lsm_egress_violator, NULL);
    pthread_create(&xdp_thread, NULL, xdp_ingress_flooder, NULL);

    pthread_join(lsm_thread, NULL);
    pthread_join(xdp_thread, NULL);

    printf("==========================================\n");
    printf("[*] Attack sequence complete. Verify /metrics.\n");
    return 0;
}
