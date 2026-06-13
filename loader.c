#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <elf.h>

/* * The iterator function
 * map_start: The virtual memory address where the ELF file is mapped
 * func: The callback function to apply to each program header
 * arg: An additional argument to pass to the callback (unused in Task 0)
 */
int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg) {
    // 1. Cast the mapped memory to an ELF header structure
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;

    // 2. Validate that the file is actually an ELF file using the magic numbers
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "Error: Not a valid ELF file\n");
        return -1;
    }

    // 3. Find the Program Header Table
    // The offset to the program headers (e_phoff) is in bytes, so we cast 
    // map_start to (char *) before adding the offset to do byte-level arithmetic.
    Elf32_Phdr *phdr_table = (Elf32_Phdr *)((char *)map_start + ehdr->e_phoff);

    // 4. Iterate over the number of program headers (e_phnum)
    for (int i = 0; i < ehdr->e_phnum; i++) {
        // Apply the provided function to each Program Header pointer
        func(&phdr_table[i], arg);
    }

    return 0;
}

/*
 * The callback function to print information about each program header.
 * We use a static variable to track the index 'i' since the 'arg' parameter 
 * is reserved for later tasks.
 */
void print_phdr_info(Elf32_Phdr *phdr, int arg) {
    static int index = 0;
    printf("Program header number %d at address %p\n", index++, (void *)phdr);
}

int main(int argc, char **argv) {
    // 1. Verify command-line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <32-bit-elf-executable>\n", argv[0]);
        return 1;
    }

    // 2. Open the ELF file
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Failed to open file");
        return 1;
    }

    // 3. Get the file size using fstat (needed for mmap)
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("Failed to get file stats");
        close(fd);
        return 1;
    }

    // 4. Map the entire file into memory
    // MAP_PRIVATE ensures we only read the file and don't modify the actual file on disk.
    void *map_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return 1;
    }

    // 5. Run the iterator over the mapped file
    // We pass 0 for 'arg' since we don't need it for this task.
    foreach_phdr(map_start, print_phdr_info, 0);

    // 6. Cleanup
    munmap(map_start, st.st_size);
    close(fd);

    return 0;
}