# Compiler settings
ASM=nasm
CC=gcc
LD=ld

# Flags
ASMFLAGS=-f elf32
CFLAGS=-m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs
LDFLAGS=-m elf_i386 -T link.ld

# Directories
BOOT_DIR=boot
KERNEL_DIR=kernel
DRIVERS_DIR=drivers
PLATFORM_DIR=platform

# Source files
BOOT_SOURCES=$(wildcard $(BOOT_DIR)/*.asm)
KERNEL_SOURCES=$(wildcard $(KERNEL_DIR)/*.c)
DRIVER_SOURCES=$(wildcard $(DRIVERS_DIR)/*.c)
PLATFORM_SOURCES=$(wildcard $(PLATFORM_DIR)/*/*.c)

# Object files
BOOT_OBJECTS=$(BOOT_SOURCES:.asm=.o)
KERNEL_OBJECTS=$(KERNEL_SOURCES:.c=.o)
DRIVER_OBJECTS=$(DRIVER_SOURCES:.c=.o)
PLATFORM_OBJECTS=$(PLATFORM_SOURCES:.c=.o)

# Output
KERNEL=kernel.bin
ISO=os.iso

all: $(KERNEL) $(ISO)

$(KERNEL): $(BOOT_OBJECTS) $(KERNEL_OBJECTS) $(DRIVER_OBJECTS) $(PLATFORM_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(ISO): $(KERNEL)
	mkdir -p isodir/boot/grub
	cp $(KERNEL) isodir/boot/
	cp grub.cfg isodir/boot/grub/
	grub-mkrescue -o $@ isodir

clean:
	rm -f $(BOOT_OBJECTS) $(KERNEL_OBJECTS) $(DRIVER_OBJECTS) $(PLATFORM_OBJECTS) $(KERNEL) $(ISO)
	rm -rf isodir

.PHONY: all clean
