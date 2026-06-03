# Sentinel SMM

> **Status:** Experimental prototype (alpha)

Sentinel SMM (System Management Mode) provides a hardware-enforced root of trust operating at Ring -2. By executing from SMRAM, it isolates critical cryptographic materials and system integrity verification logic from both the OS and the hypervisor.

## Threat Model

Threat model: We assume untrusted firmware may attempt rootkit injection in SMM; Sentinel-SMM intercepts SMIs to enforce policy, but cannot prevent physical attacks or insecure BIOS.

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
