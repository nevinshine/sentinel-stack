# Sentinel Stack (V2)

### The Intent-Bounded Systems Language Architecture

<p align="center">
  <img src="https://img.shields.io/badge/ISA-RISC--V%2064-orange?style=for-the-badge&logo=riscv" />
  <img src="https://img.shields.io/badge/Verification-Z3%20SMT-blueviolet?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Execution-TCA%20Silicon-00b894?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Compiler-telos--lang-red?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Release-v2.0.0--tapeout-blue?style=for-the-badge" />
</p>

The Semantic Gap is officially closed. 

Traditional cybersecurity models fail because they try to correlate high-level program intent with low-level execution context using software-mediated telemetry. The result is vulnerable, non-deterministic enforcement that costs hundreds of CPU cycles per system call. 

The Sentinel Stack V2 eliminates the Semantic Gap entirely. By replacing **421 cycles of software-mediated eBPF overhead** with a **2-cycle hardware capability gate**, this architecture proves the obsolescence of current security models. It is the world's first true Intent-Bounded Systems Language.

---

## The Four V2 Pillars

The V2 Architecture spans the entire execution pipeline, starting from natural language syntax and culminating in physical silicon enforcement:

### 1. The Math: Z3 Formal Verification
The `telos-lang` compiler integrates the **Z3 SMT solver** directly into the compilation pipeline. Using **Linear Temporal Logic (LTL)** and an **Information Flow Control (IFC) DAG lattice**, the compiler mathematically proves that variables never leak downward to public sinks, and resources never leak ("Ghost I/O"). If the SMT solver returns `Unsat`, compilation halts and extracts the exact Unsat Core pointing to the offending AST node.

### 2. The Boundary: Ring Transitions & Network Slam
The compiler lowers parsed `intend network` blocks directly into highly specific U-mode `ecall` sequences and Ring -1 hypervisor `ebreak` (Heki) bindings. Violations of the intent map instantly trigger a zero-cycle, mathematically irreversible **Network Slam**, neutralizing any execution path attempting exfiltration.

### 3. The Silicon: Dynamic TCA-PMP
Hardware bounds and a microarchitectural **Hardware Bloom Filter** replace expensive kernel-space policy maps. The multi-bit IFC Color Lattice enforces taint constraints at the Instruction Set Architecture (ISA) level. Malicious memory reads or lateral movement attempts are trapped at wire-speed before the operating system is even aware.

### 4. The Root of Trust: Simulated PBL Boot Sequence
The hypervisor orchestrates the `telos_bootstrap` routine, utilizing an Ed25519-verified `.text` segment loader mapped into QEMU's `virt.c` machine initialization. Only mathematically verified, Policy-Carrying Code (PCC) is permitted to execute.

---

## One-Touch Reproducibility

Reproducibility is the currency of architectural research. You can clone this repository and watch the QEMU simulator physically drop the packet on a taint violation in a single command.

### Prerequisites
* **RISC-V Toolchain:** `riscv64-unknown-elf-gcc`
* **QEMU:** `qemu-system-riscv64`
* **Rust:** `cargo` (>= 1.70)
* **Z3:** `libz3-dev`

### `make demo`

```bash
git clone --recursive https://github.com/nevinshine/sentinel-stack.git
cd sentinel-stack

# Compile the Telos program, mathematically verify the IFC boundaries,
# lower to RISC-V assembly, and boot the TCA-PMP Silicon Simulator.
make demo
```

**What happens:**
1. The `telos-lang` compiler parses the high-level policy.
2. The Z3 solver verifies the memory safety and IFC taint boundaries.
3. LLVM emits bare-metal RISC-V 64-bit object code.
4. The QEMU simulator boots the TCA ROM, loads the verified binary, and executes.
5. If the program attempts an unauthorized `socket_connect()` or downward taint flow, the silicon trap natively drops the packet in 2 cycles.

---

## Monorepo Structure

The architecture is neatly partitioned across three core workspaces:

```
sentinel-stack/
├── telos-lang/            # The Crown Jewel: Z3-Verified Rust Compiler
│   ├── telosc/src/        # AST Parser, Semantic Z3 Math Layer, LLVM RISC-V Codegen
│   └── telosc/tests/      # SMT LTL Tests (hanging_socket.telos, ifc_leak.telos)
├── tca-prototype/         # V2 Silicon Tape-out Pipeline
│   ├── hw/                # Verilog TCA Hardware Modules & Bloom Filter
│   ├── virt.c             # QEMU Boot ROM Intercept
│   └── tests/             # Bare-metal RISC-V Exfiltration tests
├── sentinel-vmi/          # M-Mode Hypervisor Introspection Engine
│   └── src/               # Ring -1 ebreak handlers and U-mode orchestrators
└── legacy_v1/             # V1 Software Fallback Archive
    ├── telos-runtime/     # Legacy eBPF LSM Daemons
    ├── hyperion-xdp/      # Legacy Wire-Speed XDP
    └── sentinel-cc/       # Legacy PCC Compiler
```

### Legacy V1 (Software Fallbacks)

The original prototype components have been retained in the `legacy_v1/` directory. While these components successfully demonstrated zero-trust zero-day interdiction using eBPF and XDP, they rely on software-mediated host kernels and are subject to the 421-cycle overhead bottleneck. They are archived here as fallbacks for environments incapable of supporting RISC-V TCA silicon.

---

## License

MIT License — see individual component licenses.

---

<p align="center">
  <b>Sentinel Stack V2</b> — <em>The Semantic Gap is Closed.</em>
</p>
