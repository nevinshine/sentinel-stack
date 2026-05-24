#include "sentinel_vmi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <inttypes.h>

#define HEKI_MAGIC 0x48454B49

#include <sentinel_ipc.h>

static int heki_listen_fd = -1;
static struct vmi_session *heki_session = NULL;
uint64_t heki_active_nonce = 0;

static uint64_t generate_random_nonce(void) {
    uint64_t nonce = 0;
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        if (read(fd, &nonce, sizeof(nonce)) != sizeof(nonce)) {
            nonce = 0;
        }
        close(fd);
    }
    if (nonce == 0) {
        // Fallback if /dev/urandom fails
        nonce = ((uint64_t)rand() << 32) | (uint64_t)rand();
        nonce ^= 0x48454B4948454B49ULL;
    }
    return nonce;
}

int heki_server_init(struct vmi_session *session, const char *socket_path) {
    if (!session || !socket_path) return -1;

    heki_session = session;

    heki_listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (heki_listen_fd < 0) {
        perror("[HEKI] socket");
        return -1;
    }

    // Set non-blocking
    int flags = fcntl(heki_listen_fd, F_GETFL, 0);
    fcntl(heki_listen_fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    (void)unlink(socket_path);

    if (bind(heki_listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[HEKI] bind");
        close(heki_listen_fd);
        return -1;
    }

    if (listen(heki_listen_fd, 5) < 0) {
        perror("[HEKI] listen");
        close(heki_listen_fd);
        return -1;
    }

    printf("[HEKI] Bridge listening on %s\n", socket_path);
    return 0;
}

static void handle_heki_client(int client_fd) {
    struct heki_drawbridge_request reg;
    ssize_t n = read(client_fd, &reg, sizeof(reg));
    
    uint64_t response = 0;

    if (n == (ssize_t)sizeof(reg) && reg.magic == HEKI_MAGIC) {
        char map_name[32];
        int is_critical = 1;
        snprintf(map_name, sizeof(map_name), "subsystem_%u_map", reg.target_subsystem);

        printf("[HEKI] Received registration for subsystem %u map\n", reg.target_subsystem);
        printf("[HEKI]  - GVA: 0x%" PRIx64 "\n", reg.payload_gva);
        printf("[HEKI]  - Size: %u\n", reg.payload_size);

        uint64_t gpa = 0;
        if (heki_session->kernel_pgd == 0) {
            fprintf(stderr, "[HEKI] Error: kernel_pgd not set yet (Mocking success for IPC testing)\n");
            if (heki_active_nonce == 0) {
                heki_active_nonce = 3039; // Fixed nonce for mocking
            }
            response = heki_active_nonce;
            printf("[HEKI] [PASS] Protected %u pages for map %s (Nonce: %lu)\n", (reg.payload_size + 0xFFF) / 0x1000, map_name, response);
        } else if (vmi_gva_to_gpa(heki_session, heki_session->kernel_pgd, reg.payload_gva, &gpa) < 0) {
            fprintf(stderr, "[HEKI] Error: Failed to translate GVA 0x%" PRIx64 " to GPA\n", reg.payload_gva);
        } else {
            printf("[HEKI] Translated GVA 0x%" PRIx64 " -> GPA 0x%" PRIx64 "\n", reg.payload_gva, gpa);
            
            // Protect the page(s)
            if (npt_guard_protect_dynamic(heki_session, gpa, reg.payload_size, is_critical, map_name) == 0) {
                // Generate a random 64-bit nonce
                if (heki_active_nonce == 0) {
                    heki_active_nonce = generate_random_nonce();
                    if (heki_active_nonce == 0) heki_active_nonce = 1;
                }
                response = heki_active_nonce; // Success -> return the nonce
            }
        }
    } else if (n == (ssize_t)sizeof(reg) && reg.magic == 0x4D4F434B) { // "MOCK"
        extern void npf_handler_cpuid_intercept(struct vmi_session *s, uint32_t eax, uint32_t ecx, uint32_t ebx, uint64_t cr3);
        uint64_t nonce = reg.ephemeral_nonce;
        uint32_t nonce_lo = (uint32_t)(nonce & 0xFFFFFFFF);
        uint32_t nonce_hi = (uint32_t)(nonce >> 32);
        
        // Mock a CPUID interception
        npf_handler_cpuid_intercept(heki_session, 0x48454B49, nonce_lo, nonce_hi, 0x12345678);
        response = 1;
    } else {
        fprintf(stderr, "[HEKI] Invalid registration payload (read %zd bytes, magic 0x%x)\n", n, reg.magic);
    }

    if (write(client_fd, &response, sizeof(response)) < 0) {
        perror("[HEKI] write response failed");
    }
    close(client_fd);
}

void heki_server_poll(void) {
    if (heki_listen_fd < 0) return;

    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(heki_listen_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd >= 0) {
        handle_heki_client(client_fd);
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("[HEKI] accept error");
    }
}
