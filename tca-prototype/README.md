# Teleological Capability Architecture (TCA) Prototype

> **Status:** Experimental prototype (alpha)

The TCA Prototype is an RTL-simulated hardware capability gate designed to enforce semantic intent directly at the silicon level, replacing software-mediated hooks. This repository contains the bare-metal tests and capabilities to validate the RISC-V QEMU hypervisor modifications.

## Threat Model

The TCA Prototype defends against spatial and temporal capability violations and exfiltration attempts via hardware-level interdiction.
**In Scope**: Code-reuse attacks, memory spatial isolation bypasses, and data exfiltration from compromised hypervisors or kernels. 
**Assumptions**: The RTL logic and hypervisor capability provisioning are correct. The hardware boot ROM and eFuses (which provide the cryptographic Root of Trust) are secure and untampered.
**Out of Scope / Limitations**: Microarchitectural side-channel attacks (e.g., Rowhammer, Cache timing) and fault injection attacks (e.g., voltage glitching) are not currently mitigated by this specific capability boundary.

---

## Getting Started

### Prerequisites

You need a RISC-V cross-compilation toolchain and the specialized `riscv-tca-sim` QEMU emulator built locally.

- `riscv64-unknown-elf-gcc`
- `make`
- The `riscv-tca-sim` emulator built from the `sentinel-stack/riscv-tca-sim` directory.

### Build Instructions

1. Compile the bare-metal TCA test binaries:
   ```bash
   make os_isolation_test_v2
   ```
   
   If you are building the legacy or alternative tests:
   ```bash
   make all
   ```

2. The compilation will produce ELF binaries such as `os_isolation_test_v2.elf` utilizing the `boot.S` entry points.

### Testing on QEMU

Run the compiled `.elf` binaries using the TCA-enabled QEMU simulator:

```bash
../riscv-tca-sim/build/qemu-system-riscv64 \
    -machine virt \
    -cpu rv64 \
    -bios none \
    -kernel os_isolation_test_v2.elf \
    -nographic \
    -serial mon:stdio
```

You should observe the hardware successfully enforcing the TCA memory and execution boundaries natively.
