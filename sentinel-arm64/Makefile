# Sentinel Stack - Bare-metal ARM64 Hypervisor Makefile

CC = aarch64-linux-gnu-gcc
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

CFLAGS = -Wall -Wextra -O2 -ffreestanding -nostdlib -mgeneral-regs-only
LDFLAGS = -T linker.ld

OBJ_DIR = obj
BIN_DIR = bin

OBJS = $(OBJ_DIR)/boot.o \
       $(OBJ_DIR)/exceptions.o \
       $(OBJ_DIR)/hypervisor.o \
       $(OBJ_DIR)/mmu.o

TARGET_ELF = $(BIN_DIR)/sentinel_hypervisor.elf
TARGET_BIN = $(BIN_DIR)/sentinel_hypervisor.bin

all: dirs $(TARGET_BIN)

dirs:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(OBJ_DIR)/%.o: src/%.S
	$(CC) -Wall -Wextra -O2 -c $< -o $@

$(OBJ_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_ELF): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean dirs
