# Null-OS-Public
Null OS is a bare bones operating system that's a microkernel, a bootloader, and also a terminal at its core

--Here's the general method to get Null OS running: 

-Step 1: Compile and Assemble Everything
"nasm -f bin NullBootloader.asm -o NullBootloader.bin"
"nasm -f elf32 Terminal.asm -o Terminal.o"
"g++ -m32 -c NullKernel.cpp -o NullKernel.o"

-Step 2: Link Kernel together
"ld -m elf_i386 -Ttext 0x1000 --oformat binary Terminal.o NullKernel.o -o NullKernel.bin"

-Step 3: Bind the bootloader and kernel into one image
"cat NullBootloader.bin NullKernel.bin > os_image.bin"

-Step 4: Package everything into a final bootable ISO image
"mkdir -p iso_root"
"cp os_image.bin iso_root/"
"xorriso -as mkisofs -R -b os_image.bin -no-emul-boot -boot-load-size 4 -o my_os.iso iso_root"
--Note that you might need to install xorriso in order to make the ISO image
