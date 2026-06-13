# Makefile for Task 2a (Safe Compilation)

all: loader

# 1. Compile the C code into a 32-bit object file
loader.o: loader.c
	gcc -m32 -D_FILE_OFFSET_BITS=64 -g -Wall -c loader.c -o loader.o

# 2. Assemble the startup assembly file into a 32-bit object file
startup.o: startup.s
	nasm -f elf32 startup.s -o startup.o

# 3. Assemble the start assembly file into a 32-bit object file
start.o: start.s
	nasm -f elf32 start.s -o start.o

# 4. Link everything together using the custom linking script
# NOTE: This command assumes a 64-bit host compiling for 32-bit.
# If on native 32-bit, change /usr/lib32 to /usr/lib and /lib32/ to /lib/
loader: loader.o startup.o start.o
	ld -m elf_i386 -o loader loader.o startup.o start.o -L/usr/lib32 -lc -T linking_script -dynamic-linker /lib32/ld-linux.so.2

# 5. Cleanup
clean:
	rm -f loader *.o