#ifndef SENTINEL_IPC_H
#define SENTINEL_IPC_H

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
typedef uint8_t  __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;
typedef uint64_t __u64;
#endif

#define SENTINEL_EVENT_PROCESS_VIOLATION 1
#define SENTINEL_EVENT_NETWORK_DROP 2

#define HEKI_MAGIC 0x48454B49
#define HEKI_SUBSYSTEM_TELOS 1
#define HEKI_SUBSYSTEM_HYPERION 2

struct process_violation {
    __u32 parent_pid;
    __u32 _pad1;            /* Explicit 4-byte padding for 8-byte alignment */
    __u64 taint_signature;
};

struct network_drop {
    __u32 saddr;
    __u32 daddr;
    __u16 target_port;
    __u16 _pad;             /* 2-byte padding */
    __u32 _pad2;            /* 4-byte padding to match the 16-byte union size */
};

/* Unified Telemetry Payload */
struct sentinel_event_t {
    __u32 event_type;
    __u32 _padding;         /* 4-byte alignment padding */
    __u64 timestamp_ns;
    __u64 pid_tgid;
    union {
        struct process_violation process_violation;
        struct network_drop network_drop;
    } data;
} __attribute__((aligned(8)));

/* CPUID Drawbridge Request Payload */
struct heki_drawbridge_request {
    __u32 magic;
    __u32 target_subsystem;
    __u64 payload_gva;
    __u32 payload_size;
    __u64 ephemeral_nonce;
} __attribute__((packed));

#endif /* SENTINEL_IPC_H */
