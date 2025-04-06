# DataOS
an os written in c

# build and run
type make
and run these commands
mkdir -p isodir/boot/grub
cp kernel.bin isodir/boot/
cp grub.cfg isodir/boot/grub/
grub-mkrescue -o dataos.iso isodir

and run:
qemu-system-i386 -cdrom dataos.iso

# known issues
the screen may not scroll
you can't use the backspace key due to a bug

# TODO
add new commands like ls, mkdir and more
add a filesystem
