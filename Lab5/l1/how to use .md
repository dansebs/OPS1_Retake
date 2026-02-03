# Lab 0: POSIX Program Execution Environment

## Overview
This laboratory covers the fundamentals of the POSIX execution environment, including standard I/O streams, process return codes, command-line arguments, and environment variables.

---

## 🛠️ Reusable Snippets

### 1. Error Handling Macro
Always use this macro for system and library functions that set `errno`. It prints the error description, the file name, and the line number before exiting.

```c
#define ERR(source) (perror(source), \
                     fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
                     exit(EXIT_FAILURE))
```
2. Robust Argument Parsing (getopt)While basic argv indexing works, getopt is the standard POSIX way to handle flags and options.Cint opt;
int times = 1;
char *name = "Default";
```c
while ((opt = getopt(argc, argv, "n:t:")) != -1) {
    switch (opt) {
        case 'n':
            name = optarg;
            break;
        case 't':
            times = atoi(optarg);
            if (times <= 0) usage(argv[0], "-t requires a positive integer");
            break;
        default:
            usage(argv[0], "[-n name] [-t times]");
    }
}
```
3. Safe Environment AccessHelpers to handle missing environment variables or set them safely.C// Get int from env or return default
int get_env_int(const char *var_name, int default_val) {
    char *val = getenv(var_name);
    return val ? atoi(val) : default_val;
}
```c
// Set env and check for '=' errors
void set_env_safe(const char *name, const char *value) {
    if (setenv(name, value, 1)) {
        if (errno == EINVAL) ERR("setenv: name contains '='");
        ERR("setenv");
    }
}
```
💡 Key Characteristics & RulesExit Status: Processes must return an int. Use EXIT_SUCCESS (0) or EXIT_FAILURE (1) from <stdlib.h>.Standard Streams: * stdout and stderr are line-buffered; include \n to ensure the buffer flushes.Errors must always be printed to stderr.Safe Reading: Use fgets(buf, size, stdin) instead of scanf for strings to prevent buffer overflows.Buffer Sizing: To store a string of length $N$, the buffer size should be $N+2$ to account for the newline \n and the null terminator \0.Environment Inheritance: setenv and putenv modify the environment of the current process and its children. These changes do not propagate back to the parent shell.