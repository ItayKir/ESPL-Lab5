# Makefile for Task 2a (Safe Compilation)

all: loader

# 1. Compile loader.c into an object file (-c)
# We keep the _FILE_OFFSET_BITS macro fix from earlier
loader.o: loader.c
	gcc -m32 -D_FILE_OFFSET_BITS=64 -g -Wall -c loader.c -o loader.o

# 2. Link the object files using the custom linking script
# NOTE: This command assumes you are on a 64-bit OS compiling for 32-bit.
loader: loader.o
	ld -m elf_i386 -o loader loader.o startup.o start.o -L/usr/lib32 -lc -T linking_script -dynamic-linker /lib32/ld-linux.so.2

clean:
	rm -f loader loader.o