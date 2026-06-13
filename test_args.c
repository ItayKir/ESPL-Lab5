/* test_args.c - A standalone program to test argument passing */

/* Tell the compiler to use the system_call function that ALREADY exists inside start.o */
extern int system_call(int eax, int ebx, int ecx, int edx);

/* Helper to find string length for sys_write */
int string_length(char *str) {
    int i = 0;
    while(str[i] != '\0') i++;
    return i;
}

/* Helper to print strings to stdout (file descriptor 1) */
void print_string(char *str) {
    // 4 is the system call number for sys_write
    system_call(4, 1, (int)str, string_length(str)); 
}

/* * The main function!
 * It will receive the arguments directly from your loader's startup.s function.
 */
int main(int argc, char **argv) {
    print_string("\n[TARGET PROGRAM WOKE UP]\n");

    for (int i = 0; i < argc; i++) {
        print_string("Argument Detected: ");
        print_string(argv[i]);
        print_string("\n");
    }

    print_string("[TARGET PROGRAM FINISHED]\n\n");
    return 0;
}