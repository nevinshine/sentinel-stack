#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/kvm.h>
#include <asm/kvm.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#define MMIO_MAP_PATH "/sys/fs/bpf/sentinel_mmio_policy"

/* LPM Trie Key Structure */
struct sentinel_mmio_key {
    uint32_t prefixlen;
    uint64_t phys_addr;
};

/* Policy Action Bitmasks */
#define SENTINEL_MMIO_ALLOW 0
#define SENTINEL_MMIO_DENY  1
#define SENTINEL_MMIO_LOG   2
#define SENTINEL_MMIO_ABORT 4

/* Core register encoding helper for ARM64 KVM */
#define ARM64_CORE_REG(x) (KVM_REG_ARM64 | KVM_REG_SIZE_U64 | KVM_REG_ARM_CORE | (x))
#define KVM_REG_ARM64_PC  ARM64_CORE_REG(2 * 32) // Program Counter offset in kvm_regs

static void advance_guest_pc(int vcpu_fd) {
    uint64_t pc_val;
    struct kvm_one_reg reg;

    reg.id = KVM_REG_ARM64_PC;
    reg.addr = (uintptr_t)&pc_val;

    if (ioctl(vcpu_fd, KVM_GET_ONE_REG, &reg) >= 0) {
        pc_val += 4; // Skip the 4-byte standard A64 instruction
        ioctl(vcpu_fd, KVM_SET_ONE_REG, &reg);
    }
}

static void inject_data_abort(int vcpu_fd) {
    struct kvm_vcpu_events events;

    memset(&events, 0, sizeof(events));
    if (ioctl(vcpu_fd, KVM_GET_VCPU_EVENTS, &events) >= 0) {
        /* Inject a synchronous administrative exception event */
        events.exception.serror_pending = 1;
        events.exception.serror_has_esr = 1;
        events.exception.serror_esr = 0x96000044; // Data Abort from lower EL
        ioctl(vcpu_fd, KVM_SET_VCPU_EVENTS, &events);
        printf("[+] Interdiction: Synchronous Data Abort exception line injected.\n");
    }
}

void sentinel_vcpu_run(int vcpu_fd, struct kvm_run *run) {
    int map_fd;
    struct sentinel_mmio_key key;
    uint32_t policy_action;

    printf("[*] Initializing Ring -1 Interdiction Control Plane...\n");
    
    /* Retrieve file descriptor from pinned BPF File System */
    map_fd = bpf_obj_get(MMIO_MAP_PATH);
    if (map_fd < 0) {
        perror("[-] Failed to retrieve pinned LPM policy map, running in pass-through mode");
    }

    while (1) {
        if (ioctl(vcpu_fd, KVM_RUN, 0) < 0) {
            perror("[-] KVM_RUN execution loop failure");
            break;
        }

        switch (run->exit_reason) {
            case KVM_EXIT_MMIO:
                policy_action = SENTINEL_MMIO_ALLOW; // Default pass-through
                
                if (map_fd >= 0) {
                    /* Construct lookup key for exact match within the LPM Trie prefix tree */
                    key.prefixlen = 64; 
                    key.phys_addr = run->mmio.phys_addr;

                    if (bpf_map_lookup_elem(map_fd, &key, &policy_action) < 0) {
                        policy_action = SENTINEL_MMIO_ALLOW; // Fallback
                    }
                }

                printf("[VM-EXIT] MMIO trap at physical address: 0x%llx [Len: %d, IsWrite: %d]\n",
                       run->mmio.phys_addr, run->mmio.len, run->mmio.is_write);

                if (policy_action & SENTINEL_MMIO_DENY) {
                    if (policy_action & SENTINEL_MMIO_ABORT) {
                        /* Terminate the malicious context inside the guest kernel via exception */
                        inject_data_abort(vcpu_fd);
                    } else {
                        /* Silent drop: clear the read buffer if a device probe, advance past instruction */
                        if (!run->mmio.is_write) {
                            memset(run->mmio.data, 0, run->mmio.len);
                        }
                        printf("[DENY] Silent instruction drop enforced.\n");
                        advance_guest_pc(vcpu_fd);
                    }
                } else {
                    /* Legitimate hardware access: execute transaction against physical address space */
                    if (policy_action & SENTINEL_MMIO_LOG) {
                        printf("[AUDIT] Authorized transaction handled.\n");
                    }
                    
                    /* Advance instruction pointer to prevent infinite re-faulting loop */
                    advance_guest_pc(vcpu_fd);
                }
                break;

            case KVM_EXIT_SYSTEM_EVENT:
                printf("[*] Guest requested clean shutdown or reboot sequence.\n");
                return;

            default:
                break;
        }
    }
}
