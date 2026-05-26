#ifndef _SENTINEL_SMM_CPU_CONTEXT_H_
#define _SENTINEL_SMM_CPU_CONTEXT_H_

#include <Uefi.h>

// This structure matches the exact order of registers pushed by
// SentinelSmmTrap.nasm during the #GP context switch.
typedef struct {
    UINT64 R15;
    UINT64 R14;
    UINT64 R13;
    UINT64 R12;
    UINT64 R11;
    UINT64 R10;
    UINT64 R9;
    UINT64 R8;
    UINT64 Rdi;
    UINT64 Rsi;
    UINT64 Rbp;
    UINT64 Rdx;
    UINT64 Rcx;
    UINT64 Rbx;
    UINT64 Rax;
} SENTINEL_CPU_CONTEXT;

#endif // _SENTINEL_SMM_CPU_CONTEXT_H_
