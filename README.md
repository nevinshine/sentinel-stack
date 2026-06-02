# Sentinel Stack (V2)

### Exploratory Hardware-Software Architecture for Intent-Bounded Execution

<p align="center">
  <img src="https://img.shields.io/badge/ISA-RISC--V%2064-orange?style=for-the-badge&logo=riscv" />
  <img src="https://img.shields.io/badge/Verification-Z3%20SMT-blueviolet?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Execution-RTL%20Prototype-00b894?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Compiler-telos--lang-red?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Release-v2.0.0--rtl--prototype-blue?style=for-the-badge" />
  <img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" />
</p>

The Sentinel Stack is an experimental systems architecture designed to constrain the semantic gap between high-level program intent and low-level hardware execution.

Traditional cybersecurity models often attempt to correlate high-level program intent with low-level execution context using software-mediated telemetry (e.g., eBPF, LSMs). This can result in non-deterministic enforcement and measurable runtime overhead. The Sentinel Stack explores hardware-bounded intent enforcement, replacing software-mediated hooks with an RTL-simulated hardware capability gate, intended to provide an order-of-magnitude reduction in mediation overhead.

---

## What is the Sentinel Stack? (The Simple Version)

Imagine a high-security manufacturing plant. In a traditional system, an inspector (the operating system kernel) stands at every door checking badges (permissions) whenever an employee (a program) tries to move a sensitive component. This slows down the entire factory.

**The Sentinel Stack changes the physics of the factory.** Instead of relying on an inspector at the door, the rules are mathematically proven before the employee even enters the building, and the physical doors themselves are wired to recognize unauthorized components. 

When a developer writes code in the Sentinel Stack, they declare their exact intent in natural language:
> *"I intend to read a secure cryptographic key and hash it."*

1. **The Math Layer:** Before the code is even compiled, a mathematical solver (Z3 SMT) proves that the developer hasn't accidentally written code that leaks the key to the internet. If the math doesn't check out, the program refuses to compile.
2. **The Hardware Layer:** Once compiled, the program is handed a cryptographic "intent receipt." When the program runs on the physical processor, a custom hardware gate checks the receipt in **exactly two clock cycles**. 

If the program attempts to break its promise—say, by opening a network socket to exfiltrate the key—the physical silicon instantly drops the connection. The operating system isn't even asked for permission. The enforcement happens in the bare metal.

---

## Architecture & Technical Deep Dive

The V2 Architecture prototypes an intent-bounded pipeline spanning from compiler semantics to a virtualized hardware model. 

```mermaid
graph TD
    classDef userSpace fill:#1e1e1e,stroke:#3776AB,stroke-width:2px,color:#fff
    classDef compSpace fill:#1e1e1e,stroke:#D22128,stroke-width:2px,color:#fff
    classDef hwSpace fill:#1e1e1e,stroke:#00b894,stroke-width:2px,color:#fff

    subgraph Compile_Time ["Semantic Verification Plane"]
        A["Telos Source Code"] -->|Parse AST| B["telos-lang Compiler"]
        B <-->|Extract Unsat Core| Z3[("Z3 SMT Solver")]
        B -->|IFC Validated IR| C["LLVM RISC-V Codegen"]
    end

    subgraph Execution_Environment ["Bare-Metal Runtime"]
        C -->|PCC Binary + Intent Hash| E["QEMU virt.c Boot ROM"]
        E -->|M-Mode Hand-off| F["Sentinel Hypervisor (Ring -1)"]
        F -->|U-Mode Execution| G["Verified Telos Process"]
    end

    subgraph Hardware_Enforcement ["Silicon Interdiction (TCA-PMP)"]
        G -.->|Unauthorized Access| H["TCA Hardware Bloom Filter"]
        H -->|Cycle 1: Hash Probe| I{"Intent Match?"}
        I -->|Miss| J(("Hardware Trap"))
        I -->|Hit| K(("Execute"))
    end

    class A,G userSpace
    class B,Z3,C compSpace
    class E,F,H,I,J,K hwSpace
```

### 1. Bounded Formal Verification (Z3 SMT)
The `telos-lang` compiler integrates the **Z3 SMT solver** directly into the compilation pipeline. Using **Linear Temporal Logic (LTL)** constraints and an **Information Flow Control (IFC)** lattice, the compiler asserts that specific taint propagation rules hold. If the SMT solver identifies a violation, compilation halts and extracts an Unsat Core pointing to the exact line in the AST where the constraint failed. 

### 2. Privilege Transitions & Enforcement
The compiler lowers parsed `intend network` blocks into highly specific U-mode `ecall` sequences and hypervisor-level `ebreak` (Heki) bindings. Policy violations trigger a deterministic, hardware-enforced trap path designed to interdict unauthorized execution at bounded latency without software mediation.

### 3. The Silicon Model (Dynamic TCA-PMP)
A virtualized hardware model, the **Teleological Capability Architecture (TCA)**, utilizes a microarchitectural hardware Bloom filter to enforce constraints outside the kernel space. The multi-bit IFC Color Lattice enforces taint restrictions at the Instruction Set Architecture (ISA) level within the RTL simulation.

### 4. Root of Trust & Boot Sequence
The hypervisor orchestrates the `telos_bootstrap` routine, utilizing an Ed25519-verified `.text` segment loader mapped into QEMU's `virt.c` machine initialization. This models an environment where only constraint-verified, Policy-Carrying Code (PCC) is permitted to execute on the processor.

---

## Features Matrix

| Capability | Sentinel V2 Implementation | Traditional Alternative |
|:-----------|:---------------------------|:------------------------|
| **Policy Generation** | Natural Language AST Parsing | Manual YAML/JSON configs |
| **Verification** | Z3 SMT Mathematical Bounds | Runtime heuristic monitoring |
| **Execution Boundary** | Hardware Capability Gate (TCA-PMP) | eBPF/LSM Syscall Interception |
| **Overhead** | ~2 Cycles (RTL Simulated) | ~400+ Cycles (Context switches) |
| **Data Tracking** | Hardware-level Taint (IFC) | User-space memory scanning |
| **Interdiction** | Deterministic Silicon Trap | Asynchronous `SIGKILL` |

---

## Preliminary Benchmarks (RTL Simulation)

To quantify the architectural advantage of hardware-bounded intent enforcement, preliminary microarchitectural benchmarks were conducted over the RTL simulator. 

**Empirical Setup:**
* **Architecture**: RISC-V 64-bit (rv64gc)
* **Environment**: Bare-metal execution on QEMU `virt` machine
* **Measurement**: Deterministic `-icount shift=0` mapping via `mcycle` CSR reads

| Operation Context | Cycle Count | Absolute Overhead vs Baseline |
| :--- | :--- | :--- |
| **Baseline Load** (No Security Gate) | 16 cycles | `0 cycles` (Reference) |
| **TCA Native Store** (Hardware Inline Gate) | 18 cycles | `+2 cycles` |
| **Simulated eBPF LSM Hook** (Software BPF Trampoline) | 437 cycles | `+421 cycles` |

**Analysis:**
The simulated eBPF hook incurred an overhead of **421 cycles**, serving as a highly conservative baseline that ignores real-world kernel penalties like context-switching and RCU locks. By contrast, the TCA hardware memory gate—evaluating the capability directly within the `EX/MEM` pipeline stages—incurred an absolute overhead of just **2 cycles**. This provides empirical justification for shifting the semantic boundary into physical silicon.

---

## Research Status

The Sentinel Stack is an exploratory research project. The components represent varying levels of maturity:

| Component                      | Status         | Notes                                                |
| ------------------------------ | -------------- | ---------------------------------------------------- |
| `telos-lang` parser            | Implemented    | High-level AST generation from natural syntax.       |
| Z3 IFC checks                  | Experimental   | Bounded verification of taint flows over the AST.    |
| LLVM RISC-V backend            | Prototype      | Emits bare-metal RISC-V object code.                 |
| TCA RTL modules                | Simulated      | Verilog Bloom filter logic synthesized in QEMU.      |
| Hypervisor integration         | Partial        | Basic M-Mode ebreak routing functioning.             |
| Hardware Bloom filter          | RTL simulation | Validated in QEMU virt machine initialization.       |
| End-to-end formal verification | Incomplete     | Requires full state-transition proof across layers.  |

---

## Scope & Threat Model

Real systems architectures require constrained and explicit boundaries. The Sentinel Stack operates under the following threat model:

**Assumptions & Trust Boundaries:**
* The boot ROM is trusted and uncompromised.
* The compiler pipeline and its SMT constraints are correctly formulated.
* The physical execution environment correctly implements the specified RTL logic.
* The hypervisor/firmware layer (M-Mode/SMM) maintains integrity over the U-mode executing environment.

**In-Scope Defenses:**
* Syscall-mediated exfiltration attempts.
* Compile-time identifiable memory safety violations.
* Bounded Information Flow Control (IFC) across explicitly defined taint domains.

**Explicit Non-Goals (Out of Scope):**
* Speculative execution side channels (e.g., Spectre, Meltdown).
* Direct Memory Access (DMA) capable adversaries.
* Microarchitectural timing and covert channel leakage.
* Undefined behavior outside the explicit Z3 constraint boundaries.

---

## Don't Trust, Verify

This architecture emphasizes reproducibility. I invite you to audit the constraints, review the compiler, and run the simulated execution paths:

### `make demo` Execution Trace

```bash
git clone --recursive https://github.com/nevinshine/sentinel-stack.git
cd sentinel-stack
make demo
```

**What happens under the hood:**
1. **Compilation:** The `telos-lang` compiler parses the high-level policy.
2. **Formal Verification:** The Z3 solver evaluates the Information Flow Control (IFC) lattice. If a leak is detected, it extracts the `Unsat Core` and aborts.
3. **Lowering:** LLVM emits bare-metal RISC-V 64-bit object code embedded with the intent capability hash.
4. **Boot Sequence:** The QEMU simulator boots the TCA ROM, loads the verified binary, and drops into U-mode.
5. **Execution:** The program runs. If it attempts an unauthorized access outside the scope of its intent hash, the hardware Bloom filter misses, trapping the instruction at the silicon layer.

---

## Prior Art & Citations

The Sentinel Stack builds upon decades of foundational systems security and formal methods research. This project explores the convergence of concepts from:

* **Proof-Carrying Code (PCC)**: Necula (1997) - Binding verifiable proofs to executable artifacts.
* **Capability-Based Security & CHERI**: Watson et al. (2015) - Hardware-enforced compartmentalization and capability architectures.
* **seL4 Microkernel**: Klein et al. (2009) - Formal verification of kernel-level state transitions.
* **Information Flow Control (IFC)**: Denning (1976) - Lattice models of secure information flow.
* **Language-Based Security**: Schneider et al. (2001) - Enforcing security policies at the compiler level.

---

## Monorepo Structure

```
sentinel-stack/
├── telos-lang/            # The Crown Jewel: Z3-Verified Rust Compiler
│   ├── telosc/src/        # AST Parser, Semantic Z3 Math Layer, LLVM RISC-V Codegen
│   └── telosc/tests/      # SMT LTL Tests (hanging_socket.telos, ifc_leak.telos)
├── tca-prototype/         # V2 Silicon RTL Pipeline
│   ├── hw/                # Verilog TCA Hardware Modules & Bloom Filter
│   ├── virt.c             # QEMU Boot ROM Intercept
│   └── tests/             # Bare-metal RISC-V Exfiltration tests
├── sentinel-vmi/          # M-Mode Hypervisor Introspection Engine
│   └── src/               # M-Mode ebreak handlers and U-mode orchestrators
└── legacy_v1/             # V1 Software Fallback Archive
    ├── telos-runtime/     # Legacy eBPF LSM Daemons
    ├── hyperion-xdp/      # Legacy Wire-Speed XDP
    └── sentinel-cc/       # Legacy PCC Compiler
```

---

## License

MIT License — see individual component licenses.
