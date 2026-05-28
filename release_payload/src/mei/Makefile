# SPDX-License-Identifier: GPL-2.0-only
#
# Sentinel Ring -3 MEI Telemetry & Interdiction Module
#
# Build against the running kernel:
#   make
#
# Build against a specific kernel:
#   make KDIR=/path/to/kernel/build
#
# Clean:
#   make clean
#
# Load:
#   sudo insmod sentinel_mei_hook.ko [strict_mkhi=1] [log_allowed=1]
#
# Unload:
#   sudo rmmod sentinel_mei_hook
#
# Verify:
#   dmesg | grep SENTINEL_MEI
#

obj-m := sentinel_mei_hook.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# Compiler flags for hardened, production-grade kernel modules
ccflags-y := -Wall -Werror -Wextra -Wno-unused-parameter
ccflags-y += -DDEBUG  # Enable pr_debug() output; remove for release builds

.PHONY: all clean load unload check utils

all: utils
	$(MAKE) -C $(KDIR) M=$(PWD) modules

utils:
	$(CC) -O2 -Wall -Wextra -o sentinel_me_disable sentinel_me_disable.c sentinel_bootguard_check.c
	$(CC) -O2 -Wall -Wextra -o mock_mei_attack mock_mei_attack.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f sentinel_me_disable mock_mei_attack


# Convenience targets for rapid development iteration
load: all
	sudo insmod sentinel_mei_hook.ko
	@echo "--- Sentinel MEI Hook loaded ---"
	@dmesg | tail -20

unload:
	sudo rmmod sentinel_mei_hook || true
	@echo "--- Sentinel MEI Hook unloaded ---"
	@dmesg | tail -10

# Quick check for MEI telemetry events in dmesg
check:
	@dmesg | grep -i SENTINEL_MEI | tail -30
