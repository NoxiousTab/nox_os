# nox_os minimal build system
# Requires: nasm, i686-elf-gcc, i686-elf-ld, qemu-system-x86_64

# Tools (override via environment)
NASM ?= nasm
CC   ?= i686-elf-gcc
LD   ?= i686-elf-ld
OBJCOPY ?= i686-elf-objcopy

# Flags
CFLAGS := -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -fno-exceptions -fno-rtti -m32
LDFLAGS := -nostdlib -m elf_i386

BUILD_DIR := build
BOOT_DIR := bootloader
KERNEL_DIR := kernel
DOCS_DIR := docs

MBR_BIN := $(BUILD_DIR)/mbr.bin
STAGE2_BIN := $(BUILD_DIR)/stage2.bin
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin
IMG := $(BUILD_DIR)/nox_os.bin
BUILD_INC := $(BUILD_DIR)/build.inc

KERNEL_SRCS := \
	$(KERNEL_DIR)/kernel.c \
	$(KERNEL_DIR)/drivers/vga.c \
	$(KERNEL_DIR)/drivers/keyboard.c \
	$(KERNEL_DIR)/cli/shell.c \
	$(KERNEL_DIR)/sys/idt.c \
	$(KERNEL_DIR)/sys/isr.c \
	$(KERNEL_DIR)/sys/pic.c \
	$(KERNEL_DIR)/sys/pit.c \
	$(KERNEL_DIR)/sys/reboot.c \
	$(KERNEL_DIR)/lib/string.c \
	$(KERNEL_DIR)/lib/mem.c

KERNEL_OBJS := $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/%.o,$(KERNEL_SRCS))
ASM_SRCS := $(KERNEL_DIR)/sys/interrupts.asm $(KERNEL_DIR)/start.asm
ASM_OBJS := $(BUILD_DIR)/sys/interrupts.o $(BUILD_DIR)/start.o

.PHONY: all clean run docs

all: $(IMG)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/drivers
	@mkdir -p $(BUILD_DIR)/sys
	@mkdir -p $(BUILD_DIR)/lib

$(MBR_BIN): $(BOOT_DIR)/mbr.asm | $(BUILD_DIR)
	$(NASM) -f bin $< -o $@

$(STAGE2_BIN): $(BOOT_DIR)/stage2.asm $(BUILD_INC) | $(BUILD_DIR)
	$(NASM) -f bin -I$(BUILD_DIR)/ $< -o $@

$(BUILD_INC): $(KERNEL_BIN) | $(BUILD_DIR)
	@echo "[INC] generating build.inc"
	@ksz=$$(stat -c %s $(KERNEL_BIN)); \
	 ksec=$$(( (ksz + 511) / 512 )); \
	 echo "%define KERNEL_SECTORS $$ksec" > $(BUILD_INC)

$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(BUILD_DIR)/sys/interrupts.o: $(KERNEL_DIR)/sys/interrupts.asm | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $@)
	$(NASM) -f elf32 $< -o $@

$(BUILD_DIR)/start.o: $(KERNEL_DIR)/start.asm | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(NASM) -f elf32 $< -o $@

$(KERNEL_ELF): $(KERNEL_OBJS) $(ASM_OBJS) $(KERNEL_DIR)/linker.ld | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -T $(KERNEL_DIR)/linker.ld -o $@ $(ASM_OBJS) $(KERNEL_OBJS)

$(KERNEL_BIN): $(KERNEL_ELF) | $(BUILD_DIR)
	$(OBJCOPY) -O binary $< $@

$(IMG): $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_BIN) | $(BUILD_DIR)
	@echo "[IMG] creating flat disk image"
	@cat $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_BIN) > $@
	@truncate -s 1M $@

run: $(IMG)
	qemu-system-x86_64 -drive format=raw,file=$(BUILD_DIR)/nox_os.bin -serial stdio -no-reboot -no-shutdown -monitor none -d guest_errors

clean:
	rm -rf $(BUILD_DIR)

# Docs placeholder
docs:
	@echo "Generate docs in $(DOCS_DIR)"
