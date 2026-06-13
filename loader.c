#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <elf.h>
#include <string.h>

/* Helper function to convert the p_type integer to a readable string */
const char* type_to_str(Elf32_Word type) {
    switch (type) {
        case PT_NULL:           return "NULL";
        case PT_LOAD:           return "LOAD";
        case PT_DYNAMIC:        return "DYNAMIC";
        case PT_INTERP:         return "INTERP";
        case PT_NOTE:           return "NOTE";
        case PT_SHLIB:          return "SHLIB";
        case PT_PHDR:           return "PHDR";
        case PT_TLS:            return "TLS";
        case PT_NUM:            return "NUM";
        case PT_GNU_EH_FRAME:   return "GNU_EH_FRAME";
        case PT_GNU_STACK:      return "GNU_STACK";
        case PT_GNU_RELRO:      return "GNU_RELRO";
        default:                return "UNKNOWN";
    }
}

/* * The iterator function from Task 0 
 */
int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg) {
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;

    // Validate ELF magic numbers
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "Error: Not a valid ELF file\n");
        return -1;
    }

    // Locate the program header table and iterate over it
    Elf32_Phdr *phdr_table = (Elf32_Phdr *)((char *)map_start + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        func(&phdr_table[i], arg);
    }

    return 0;
}

/*
 * The updated callback function for Task 1a.
 * Prints the information residing in the corresponding Elf32_Phdr structure.
 */
void print_phdr_info(Elf32_Phdr *phdr, int arg) {
    // Extract protection flags (PF_R, PF_W, PF_X)
    char r = (phdr->p_flags & PF_R) ? 'R' : ' ';
    char w = (phdr->p_flags & PF_W) ? 'W' : ' ';
    char e = (phdr->p_flags & PF_X) ? 'E' : ' '; // Execute is traditionally 'E' in readelf

    // Print formatted header information similar to readelf -l
    printf("%-7s 0x%06x 0x%08x 0x%08x 0x%05x 0x%05x %c%c%c 0x%x\n",
           type_to_str(phdr->p_type),
           phdr->p_offset,
           phdr->p_vaddr,
           phdr->p_paddr,
           phdr->p_filesz,
           phdr->p_memsz,
           r, w, e,
           phdr->p_align);
}

/*
 * Task 1b: Map ELF program header flags to mmap protection flags.
 * * p_flags: The p_flags field extracted from an Elf32_Phdr struct.
 * Returns: A bitmask of PROT_* flags suitable for the mmap() system call.
 */
int get_prot_flags(int p_flags) {
    int prot = 0; // Default to PROT_NONE (0)

    // Translate Read flag
    if (p_flags & PF_R) {
        prot |= PROT_READ;
    }
    // Translate Write flag
    if (p_flags & PF_W) {
        prot |= PROT_WRITE;
    }
    // Translate Execute flag
    if (p_flags & PF_X) {
        prot |= PROT_EXEC;
    }

    return prot;
}

/*
 * Task 2b: The loading callback function.
 * Maps PT_LOAD segments into memory at their specified virtual addresses.
 */
void load_phdr(Elf32_Phdr *phdr, int fd) {
    // 1. We ONLY care about segments that the linker marked as "LOAD"
    if (phdr->p_type != PT_LOAD) {
        return;
    }

    // 2. Translate the ELF protection flags into mmap protection flags (Task 1b)
    int prot = get_prot_flags(phdr->p_flags);

    // 3. Calculate Page-Aligned Addresses using Bitwise masks
    // 0xfffff000 zeroes out the last 3 hex digits (12 bits), rounding down to a multiple of 0x1000
    // 0xfff isolates ONLY the last 3 hex digits, giving us the exact padding amount
    Elf32_Addr vaddr = phdr->p_vaddr & 0xfffff000;
    Elf32_Off offset = phdr->p_offset & 0xfffff000;
    Elf32_Word padding = phdr->p_vaddr & 0xfff;

    // 4. Map the segment into memory
    // MAP_PRIVATE: Don't write changes back to the actual file
    // MAP_FIXED: Put it EXACTLY at the virtual address specified
    void *mapped_mem = mmap((void *)vaddr, phdr->p_memsz + padding, prot, MAP_PRIVATE | MAP_FIXED, fd, offset);

    if (mapped_mem == MAP_FAILED) {
        perror("mmap failed inside load_phdr");
        exit(1);
    }
    

    // FIX: The .bss section bug
    // If the memory size is strictly larger than the file size, the segment contains .bss data.
    // mmap pulled in file garbage for this extra space, so we must manually zero it out!
    if (phdr->p_memsz > phdr->p_filesz) {
        // Calculate exactly where the file data ends and the uninitialized data begins
        Elf32_Addr bss_start = phdr->p_vaddr + phdr->p_filesz;
        Elf32_Word bss_size = phdr->p_memsz - phdr->p_filesz;
        
        // Zero-fill the uninitialized memory to satisfy the C standard
        memset((void *)bss_start, 0, bss_size);
    }
    // Optional: Print a success message for debugging
    // print_phdr_info(phdr, 0); // You can call your Task 1a function here to trace what loaded
}

/* * Declare the assembly function provided in startup.o 
 * This function perfectly mimics the Linux kernel's initial stack setup
 * before jumping to the entry point.
 */
extern void startup(int argc, char **argv, void *start);

int main(int argc, char **argv) {
    // 1. Notice the change here: argc < 2 (instead of != 2)
    // This allows us to pass additional arguments to the loaded program!
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <32-bit-elf-executable> [arg1 arg2 ...]\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Failed to open file");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("Failed to get file stats");
        close(fd);
        return 1;
    }

    // Map the file temporarily just to read the ELF and Program Headers
    void *map_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return 1;
    }

    // 2. Execute the loader (Task 2b)
    foreach_phdr(map_start, load_phdr, fd);

    // 3. Extract the Entry Point
    // The ELF header holds e_entry, the exact virtual address where the program begins
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;
    void *entry_point = (void *)ehdr->e_entry;

    // 4. Clean up our temporary blueprint mapping
    // We unmap map_start because the target program doesn't need to read its own headers.
    // However, we DO NOT close(fd)! The segments mapped in load_phdr are actively using it.
    munmap(map_start, st.st_size);

    // 5. Hand over control! (Tasks 2c & 2d)
    // argc - 1: We subtract 1 to hide our loader's name from the target program.
    // argv + 1: We shift the array pointer forward by 1, making the target program's name its argv[0].
    startup(argc - 1, argv + 1, entry_point);

    // The startup function transitions execution to the loaded program. 
    // That program will eventually call the exit() system call itself.
    // Therefore, this line of code will absolutely never be reached!
    return 0;
}