# Operating Systems Lab: Survival Code Toolkit

**Version:** 1.0  
**Context:** POSIX C Programming (Linux)  
**Topic:** Processes, Signals, and Low-Level I/O

## 📋 Overview
This repository contains a collection of "Battle-Ready" code snippets designed for Operating Systems laboratory exams. These snippets cover signal handling, process topologies (rings, trees), file descriptor manipulation, and safe synchronization using `sigsuspend`.

## ⚙️ Global Prerequisites
For any of the snippets below to work, your C files **must** start with the following standard headers and macro definitions.

### Required Headers
```c
#define _GNU_SOURCE // CRITICAL: Enables siginfo_t, TEMP_FAILURE_RETRY, etc.
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

Required Macros & Globals
Most snippets assume the existence of these helper constructs:
code
C
// Error handling macro that terminates the process group on failure
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

// Atomic flag for signal handling synchronization
volatile sig_atomic_t last_signal = 0;
📂 File Manifest & Usage
1. gettingsender.txt (Advanced Signal Handling)
Purpose: Sets up a signal handler capable of reading the siginfo_t structure. This is required when you need to identify the PID of the process that sent the signal (e.g., a Server needing to know which Client signaled it).
Key Mechanic: act.sa_flags = SA_SIGINFO. This tells the OS to provide the siginfo_t struct to the handler.
Usage: Access info->si_pid to get the sender's PID.
⚠️ Warning: Do not use printf inside handlers in strict environments. Save the PID to a volatile sig_atomic_t global variable and print it in main.
2. usefulSmall.txt (Utility Snippets)
Purpose: A Swiss Army Knife of short, specific tools for file manipulation and signal masking.
File Existence: Uses access() to check if a file exists before opening.
Random Range: Proper seeding using srand(time(NULL) ^ getpid()) to ensure unique sequences per child.
"Open Ears" Fix: Code to unblock signals immediately after fork() if the parent had them blocked.
increment_file_counter: A robust function to Read integer 
→
→
 Increment 
→
→
 Rewind (lseek) 
→
→
 Overwrite.
⚠️ Warning: When using lseek to overwrite, NEVER open the file with O_APPEND.
3. WaitingForOneSpecific.txt (Selective Waiting)
Purpose: Used when a process (usually a Child) needs to ignore all signals except for one specific command (e.g., "Start").
Key Mechanic: Uses sigfillset to block everything, then sigdelset to allow one signal.
Usage: Passes this strict mask to sigsuspend.
⚠️ Warning: You must register a handler for the specific signal before waiting, or the default action (Termination) will kill the process.
4. recursive.txt (Tree Topology)
Purpose: Used for "Bureaucracy" or "Hierarchy" tasks. Creates a binary tree of processes (Parent 
→
→
 2 Children 
→
→
 4 Grandchildren).
Key Mechanic: Recursive function calls inside fork().
Critical: exit(EXIT_SUCCESS) inside the Child block. Without this, the child will finish the function and continue executing the Parent's code (Fork Bomb).
Cleanup: Includes a while(wait(NULL) > 0) loop to ensure no zombies are left behind.
5. WaitingForSignals.txt (The Event Loop)
Purpose: The standard "Parent Router" loop. It waits for signals indefinitely until a specific termination signal (SIGINT) is received.
Key Mechanic: sigsuspend(&oldmask). Atomically unblocks signals and waits.
Logic: Wait 
→
→
 Wake up 
→
→
 Check last_signal 
→
→
 Act 
→
→
 Reset.
⚠️ Warning: Signals must be blocked via sigprocmask before entering this loop to prevent race conditions.
🛠️ Compilation
When compiling these snippets, use the following flags to ensure strict standard compliance and catch memory errors:
code
Bash
gcc -Wall -Wextra -fsanitize=address,undefined -o program program.c
📝 Final Exam Tips
Always Disable Buffering: Add setbuf(stdout, NULL); at the start of main to prevent print outputs from appearing out of order.
Zombie Reaper: Always handle SIGCHLD or put a while(wait...) loop at the end of the parent process.
Atomic Flags: Only use volatile sig_atomic_t variables for communication between signal handlers and the main loop.
code
Code
