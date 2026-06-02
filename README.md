# Sentinel Stack (V2)

### Exploratory Hardware-Software Architecture for Intent-Bounded Execution

<p align="center">
  <img src="https://img.shields.io/badge/ISA-RISC--V%2064-orange?style=for-the-badge&logo=riscv" />
  <img src="https://img.shields.io/badge/Verification-Z3%20SMT-blueviolet?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Execution-RTL%20Prototype-00b894?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Compiler-telos--lang-red?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Release-v2.0.0--rtl--prototype-blue?style=for-the-badge" />
</p>

The Sentinel Stack is an experimental systems architecture designed to constrain the semantic gap between high-level program intent and low-level hardware execution.

Traditional cybersecurity models often attempt to correlate high-level program intent with low-level execution context using software-mediated telemetry. This can result in non-deterministic enforcement and measurable runtime overhead. The Sentinel Stack explores hardware-bounded intent enforcement, replacing software-mediated eBPF hooks with an RTL-simulated hardware capability gate, intended to provide an order-of-magnitude reduction in mediation overhead.

---

## The Four Architecture Pillars

The V2 Architecture prototypes an intent-bounded pipeline spanning from compiler semantics to a virtualized hardware model:

### 1. Bounded Formal Verification (Z3 SMT)
The `telos-lang` compiler integrates the **Z3 SMT solver** directly into the compilation pipeline. Using **Linear Temporal Logic (LTL)** constraints and an **Information Flow Control (IFC)** lattice, the compiler asserts that specific taint propagation rules hold. If the SMT solver identifies a violation, compilation halts and extracts an Unsat Core pointing to the constraint failure. 

### 2. Privilege Transitions & Enforcement
The compiler lowers parsed `intend network` blocks into specific U-mode `ecall` sequences and hypervisor-level `ebreak` (Heki) bindings. Policy violations trigger a deterministic, hardware-enforced trap path designed to interdict unauthorized execution at bounded latency.

### 3. The Silicon Model (Dynamic TCA-PMP)
A virtualized hardware model (Teleological Capability Architecture) utilizes a hardware Bloom filter to enforce constraints outside the kernel space. The multi-bit IFC Color Lattice enforces taint restrictions at the Instruction Set Architecture (ISA) level within the RTL simulation.

### 4. Root of Trust & Boot Sequence
The hypervisor orchestrates the `telos_bootstrap` routine, utilizing an Ed25519-verified `.text` segment loader mapped into QEMU's `virt.c` machine initialization. This models an environment where only constraint-verified, Policy-Carrying Code (PCC) is permitted to execute.

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

* **The Math Layer (Z3 SMT)**: Examine the `telos-lang` compiler to see how the Information Flow Control (IFC) lattice extracts an `Unsat Core` upon constraint failure.
* **The Hardware Layer (RISC-V)**: Run the RTL-simulated prototype. Boot the QEMU simulator and observe the simulated hardware trap path interdicting an unverified execution flow.

**Run the RTL Prototype:**
```bash
git clone --recursive https://github.com/nevinshine/sentinel-stack.git
cd sentinel-stack
make demo
```

---

## Prior Art & Citations

The Sentinel Stack builds upon decades of foundational systems security and formal methods research. This project explores the convergence of concepts from:

* **Proof-Carrying Code (PCC)**: Necula (1997) - Binding verifiable proofs to executable artifacts.
* **Capability-Based Security & CHERI**: Watson et al. (2015) - Hardware-enforced compartmentalization and capability architectures.
* **seL4 Microkernel**: Klein et al. (2009) - Formal verification of kernel-level state transitions.
* **Information Flow Control (IFC)**: Denning (1976) - Lattice models of secure information flow.
* **Language-Based Security**: Schneider et al. (2001) - Enforcing security policies at the compiler level.

---

## License

MIT License — see individual component licenses.
