#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <elf.h>

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

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <32-bit-elf-executable>\n", argv[0]);
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

    void *map_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return 1;
    }

    // Print the column headers for the output
    printf("%-7s %-8s %-10s %-10s %-7s %-7s %-3s %s\n",
           "Type", "Offset", "VirtAddr", "PhysAddr", "FileSiz", "MemSiz", "Flg", "Align");

    // Execute the iterator with the new Task 1a callback
    foreach_phdr(map_start, print_phdr_info, 0);

    munmap(map_start, st.st_size);
    close(fd);

    return 0;
}