#!/bin/bash

# Launch the bare-metal Sentinel Hypervisor in QEMU
# - virtualization=on: Boot starting at EL2
# - d int,guest_errors: Log hardware exceptions to qemu_execution.log

qemu-system-aarch64 \
    -M virt,virtualization=on,secure=off \
    -cpu cortex-a57 \
    -smp 1 \
    -m 1024 \
    -nographic \
    -kernel bin/sentinel_hypervisor.elf \
    -d int,guest_errors \
    -D qemu_execution.log
