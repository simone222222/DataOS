# Compiler and linker settings
ASM=nasm
CC=gcc
LD=ld

# Flags
ASMFLAGS=-f elf32
CFLAGS=-m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c
LDFLAGS=-m elf_i386 -T linker.ld -nostdlib

# Source files
ASM_SOURCES=multiboot_header.asm boot.asm
C_SOURCES=kernel.c

# Object files
ASM_OBJECTS=$(ASM_SOURCES:.asm=.o)
C_OBJECTS=$(C_SOURCES:.c=.o)
OBJECTS=$(ASM_OBJECTS) $(C_OBJECTS)

# Output files
KERNEL=kernel.bin
ISO=dataos.iso

# Default target
all: $(KERNEL)

# Rule to build the kernel
$(KERNEL): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

# Rule to build assembly files
%.o: %.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

# Rule to build C files
%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

# Rule to create an ISO image
iso: $(KERNEL)
	mkdir -p isodir/boot/grub
	cp $(KERNEL) isodir/boot/
	cp grub.cfg isodir/boot/grub/
	grub-mkrescue -o $(ISO) isodir

# Clean up
clean:
	rm -f $(OBJECTS) $(KERNEL) $(ISO)
	rm -rf isodir

# Run in QEMU
run: iso
	qemu-system-i386 -cdrom $(ISO)

.PHONY: all iso clean run