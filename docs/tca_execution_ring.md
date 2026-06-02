# The Teleological Capability Architecture (TCA) Execution Ring

The Sentinel Stack represents a paradigm shift from software-mediated security policies to mathematically verified, hardware-enforced intent boundaries. The "Execution Ring" is the complete lifecycle of a capability within this architecture, spanning from compile-time static analysis to dynamic physical silicon enforcement.

## 1. Static Verification and Compilation (`telos-lang`)
The lifecycle begins in the compiler plane. A developer writes source code defining explicit system constraints using the `intend` keyword:

```rust
intend network {
    allow tcp 0.0.0.0:443
}
```

The `telos-lang` compiler translates these natural semantics into an Information Flow Control (IFC) lattice, verified mathematically by the Z3 SMT solver. If the program violates its own constraints, the solver extracts an *Unsat Core* and rejects the compilation. 

If verified, the LLVM `SentinelPass` embeds a cryptographic capability—a 56-byte `TelosPolicyEntry` containing the Intent Hash, the spatial boundary offset (`limit_pc_offset`), and a pre-computed 256-bit Hardware Bloom Filter—directly into the `.telos_policy` section of the resulting ELF binary.

## 2. Bootstrapping the Root of Trust (`virt.c` & Boot ROM)
At boot time, the QEMU Virtual Machine Initialization phase intercepts the binary loading process. An Ed25519 signature check is performed over the `.text` segment. Only mathematically verified and signed binaries are permitted to enter the execution pipeline.

## 3. Offline Policy Extraction (`sentinel-vmi`)
The M-Mode Bare-Metal Hypervisor (`sentinel-vmi`) operates as a Type-1 Micro-Hypervisor. Upon booting, it parses the embedded U-Mode guest payload using a defensive, zero-dependency `elf_parser`. 

Because this runs in Machine Mode, memory safety is paramount. The hypervisor securely extracts the `.telos_policy` section offline, ingesting the capability constraints before ever dropping privileges.

## 4. Dynamic Hardware Provisioning (The "Drawbridge")
When the U-Mode payload is ready to execute privileged operations, it traps into the hypervisor via an `ecall`. The M-Mode orchestrator catches the trap (`mcause == 8`) and springs the "Drawbridge":

1. **Authentication:** The requested intent is matched against the statically extracted policy.
2. **Absolute Relocation:** The spatial `limit_pc` is calculated absolutely, insulating the architecture from ASLR or relocation offsets.
3. **Hardware Injection:** The constraints are pushed directly into the physical RISC-V CSRs (`CSR_TCA_INTENT`, `CSR_TCA_BLOOM`, `CSR_TCA_LIMIT_PC`).
4. **Ring Transition:** The hypervisor adjusts the `mepc`, drops `mstatus.MPP` to `00`, and executes `mret`.

## 5. Silicon Enforcement (`riscv-tca-sim`)
As the U-Mode thread executes, the TCG (Tiny Code Generator) fast-path continuously evaluates the spatial bounds parallel to instruction fetch. 

If the thread attempts to leap past the `limit_pc` (e.g., via a Return-Oriented Programming hijack or legitimate lifecycle completion), the hardware deterministically zeroes out the `tca_intent_hash` and immediately flushes the Translation Lookaside Buffer (TLB).

Any subsequent attempt to access restricted memory triggers a `[TCA SILICON FAULT] Semantic Intent Violation` and raises a `STORE_PAGE_FAULT`, instantly interdicting the attack with single-digit cycle overhead.
