# Sentinel SMM

> **Status:** Experimental prototype (alpha)

Sentinel SMM (System Management Mode) provides a hardware-enforced root of trust operating at Ring -2. By executing from SMRAM, it isolates critical cryptographic materials and system integrity verification logic from both the OS and the hypervisor.

## Threat Model

Sentinel SMM protects the fundamental integrity of the hardware and early boot stages from persistent threats.
**In Scope**: Malicious firmware implants, UEFI rootkits, and Ring 0 exploits attempting to corrupt SMM memory or subvert the boot process.
**Assumptions**: The fundamental hardware root of trust (e.g., CPU microcode, eFuses) is intact. SMM memory (SMRAM) is properly locked by the chipset before booting the OS (e.g., D_LCK bit is set).
**Out of Scope / Limitations**: Physical hardware supply chain attacks (e.g., malicious silicon or compromised physical SPI flash chips manipulated via hardware programmers) are out of scope.

---

## Getting Started

### Prerequisites

To build the SMM modules, you need the EDK2 build environment configured on your host.

- EDK2 (TianoCore) Toolkit
- `gcc` / `clang` (or MSVC if on Windows)
- `make`
- Python 3.x (for EDK2 BaseTools)
- `nasm`

### Build Instructions

1. Set up the EDK2 workspace environment:
   ```bash
   export WORKSPACE=/path/to/your/edk2
   source edk2/edksetup.sh
   ```

2. Symlink or copy the `sentinel-smm` directory into your EDK2 workspace (e.g., under `MdeModulePkg` or as a standalone package).

3. Compile the SMM driver using the `build` command, targeting the `SentinelSmm.dsc` configuration:
   ```bash
   build -a X64 -p sentinel-smm/SentinelSmm.dsc -t GCC5 -b RELEASE
   ```

4. The resulting `.efi` module will be located in your `Build/` directory, ready to be flashed or injected into a QEMU OVMF image for testing.

### Testing in QEMU

You can test the built `.efi` module using QEMU with the OVMF firmware image.

```bash
qemu-system-x86_64 \
  -machine q35,smm=on \
  -global driver=cfi.pflash01,property=secure,value=on \
  -drive if=pflash,format=raw,unit=0,file=OVMF_CODE.fd,readonly=on \
  -drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd \
  -debugcon file:debug.log -global isa-debugcon.iobase=0x402
```
