# How to Use Reusable Modules

This directory contains a collection of reusable C modules and snippets designed to assist with Operating Systems labs, particularly for synchronization, threading, and file I/O tasks.

## 📂 Directory Structure

|- **`producer_consumer_core.c`**: Core logic for Producer-Consumer problems using pthreads.
|- **`read_files_helper.c`**: Utilities for reading specific lines from files (first, last, nth, by ID).
|- **`files_helper.c`**: General file handling helpers.
|- **`synchro_patterns.txt`**: Code snippets for common synchronization patterns (e.g., Barriers).
|- **Subdirectories** (`barrier`, `mutex`, `semaphores`, `condvariables`, `signals`, `threads`):
  Each contains specialized helper functions (`helper.c`) and toolkits (`toolkit.c`) for their respective topics.

---

## 🚀 When and How to Use

### 1. Producer-Consumer Core (`producer_consumer_core.c`)
**Use when:** You need to implement a thread pool or a producer-consumer architecture.
- Contains a `thread_pool_t` struct with built-in mutex and condition variables.
- `worker_routine` handles the waiting logic safely.
- **How to use:** Copy the struct and the worker routine into your project. Modify `task_t` to hold the specific data your threads need to process.

### 2. File I/O Helpers (`read_files_helper.c`)
**Use when:** You need to parse text files, specifically to jump to certain lines without reading the whole file into memory.
- `read_first_line(fd)`
- `read_nth_line(fd, n)`: Efficiently skips to the Nth line.
- `read_last_line(fd)`: Uses `lseek` to read from the end.
- `get_line_by_id(fd, id)`: Searches for a substring in a line.
- **How to use:** Include the functions in your file or compile them as a separate object file. Useful for configuration reading or log processing.

### 3. Synchronization Toolkits (`mutex/`, `semaphores/`, etc.)
**Use when:** You want wrapper functions or examples for specific synchronization primitives.
- **`barrier/`**: Check here if you need threads to wait for each other at a specific point.
- **`semaphores/`**: Helpers for `sem_wait` and `sem_post`.
- **`signals/`**: Signal handling setups (SIGINT, etc.).

### 4. Synchronization Patterns (`synchro_patterns.txt`)
**Use when:** You need a quick reference for implementing standard patterns.
- Currently contains a snippet for using `pthread_barrier_t` correctly (wait for N+1 threads if main thread is involved).

---

## 🛠 Quick Example: Reading a File

If you only need to read the 5th line of a file:

```c
#include "read_files_helper.c" // Or copy the function

int main() {
    int fd = open("my_data.txt", O_RDONLY);
    read_nth_line(fd, 5);
    close(fd);
    return 0;
}
```
