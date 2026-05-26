;------------------------------------------------------------------------------
; Sentinel SMM Supervisor - Trap Handler
;
; This assembly stub intercepts #GP (General Protection Faults) caused by
; CPL3 SMI handlers attempting forbidden privileged instructions (WRMSR, IN, OUT).
; It ensures strict register preservation before handing execution over to the
; C-level Policy-Shim for evaluation.
;------------------------------------------------------------------------------

SECTION .text

global SentinelGpFaultHandler
extern PolicyCheckTrapHandler

;------------------------------------------------------------------------------
; SentinelGpFaultHandler
;
; The CPU pushes the following to the stack on a #GP fault (with error code):
; [RSP + 0x28] SS (if privilege level changed)
; [RSP + 0x20] RSP (if privilege level changed)
; [RSP + 0x18] RFLAGS
; [RSP + 0x10] CS
; [RSP + 0x08] RIP
; [RSP + 0x00] Error Code
;------------------------------------------------------------------------------
SentinelGpFaultHandler:
    ; 1. Preserve all General Purpose Registers (GPRs)
    ; We are now operating on the Supervisor Stack (CPL0)
    push    rax
    push    rbx
    push    rcx
    push    rdx
    push    rbp
    push    rsi
    push    rdi
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15

    ; 2. Setup arguments for the C-level PolicyCheckTrapHandler
    ; EDK II uses the Microsoft x64 calling convention:
    ; RCX = Arg 1, RDX = Arg 2, R8 = Arg 3, R9 = Arg 4
    
    ; The Error Code is at RSP + (15 * 8) = RSP + 0x78
    ; The RIP is at RSP + 0x80
    
    mov     rcx, [rsp + 0x78]  ; Arg 1: Error Code
    mov     rdx, [rsp + 0x80]  ; Arg 2: Faulting RIP
    mov     r8,  rsp           ; Arg 3: Pointer to saved registers structure (context)

    ; 3. Ensure stack is 16-byte aligned before calling C function
    ; EDK II / MS x64 ABI requires 32 bytes of shadow space on the stack.
    mov     rbp, rsp           ; Save current stack pointer
    and     rsp, ~0x0F         ; Align stack to 16 bytes
    sub     rsp, 0x20          ; Allocate 32 bytes shadow space

    ; 4. Call the Brain (Policy Engine)
    call    PolicyCheckTrapHandler

    ; 5. Restore stack pointer
    mov     rsp, rbp

    ; Check return value (RAX). If RAX == 0, the instruction was handled/denied 
    ; and RIP was advanced by the C handler. If RAX != 0, it is a fatal violation.
    ; For now, we assume the C handler modifies the saved RIP directly if needed.

    ; 6. Restore all General Purpose Registers (GPRs)
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rdi
    pop     rsi
    pop     rbp
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax

    ; 7. Discard the Error Code from the stack
    add     rsp, 8

    ; 8. Return from Interrupt (IRETQ restores RIP, CS, RFLAGS, and RSP/SS)
    iretq
