# Raspberry Pi 3 Build Configuration

# Cross-compiler settings
CROSS_COMPILE ?= arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

# Architecture specific flags
ARCH = arm
CPU = cortex-a53
ARCH_FLAGS = -mcpu=$(CPU) -mfpu=neon-fp-armv8 -mfloat-abi=hard

# Compiler flags
CFLAGS = -Wall -O2 $(ARCH_FLAGS)
CFLAGS += -ffreestanding -nostdlib
CFLAGS += -DRPI3 -D__RASPBERRY_PI__

# Assembler flags
ASFLAGS = $(ARCH_FLAGS)

# Linker flags
LDFLAGS = -nostdlib -T rpi3.ld

# Include directories
INCLUDES = -I../include

# Output directories
BUILD_DIR = build/rpi3
KERNEL_DIR = $(BUILD_DIR)/kernel

# Kernel binary name
KERNEL = kernel7.img

# Boot files needed for RPi3
BOOT_FILES = bootcode.bin start.elf fixup.dat config.txt

# Targets
.PHONY: all clean

all: $(BUILD_DIR)/$(KERNEL)

$(BUILD_DIR)/$(KERNEL): $(KERNEL_DIR)
	$(CC) $(LDFLAGS) -o $@ $(KERNEL_DIR)/*.o

clean:
	rm -rf $(BUILD_DIR)
