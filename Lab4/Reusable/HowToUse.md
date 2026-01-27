# OPS1 Reusable Library Documentation

This directory (`Reusable/`) contains a set of C modules and wrappers designed to simplify common Operating Systems tasks such as Threading, Synchronization, File I/O, and Signal Handling.

---

## 📂 Table of Contents

1. [File I/O Helpers](#1-file-io-helpers)
2. [Synchronization Primitives](#2-synchronization-primitives)
   - [Mutexes](#mutexes)
   - [Condition Variables](#condition-variables)
   - [Semaphores](#semaphores)
   - [Barriers](#barriers)
3. [Threading Toolkit](#3-threading-toolkit)
4. [Signal Handling](#4-signal-handling)
5. [Producer-Consumer Core](#5-producer-consumer-core)
6. [Compilation Guide](#6-compilation-guide)

---

## 1. File I/O Helpers

Located in: `read_files_helper.c`, `files_helper.c`

These utilities help with reading specific parts of a file without loading the entire content into memory, and provide wrappers for `open()`.

### `read_files_helper.c` (Read Logic)

| Function | Signature | Description |
| :--- | :--- | :--- |
| **Read First** | `void read_first_line(int fd)` | Prints characters from the start until the first newline. |
| **Read Nth** | `void read_nth_line(int fd, int n)` | Skips `n-1` lines and prints the `n`-th line. 1-indexed. |
| **Read Last** | `void read_last_line(int fd)` | Seeks to the end and reads backwards to find the last line. |
| **Find by ID** | `void get_line_by_id(int fd, const char *id)` | Scans the file line-by-line and prints the line containing `id`. |

### `files_helper.c` (Open Logic)

Refer to this file for examples of flags:
- `O_RDONLY`: Read only.
- `O_WRONLY | O_CREAT | O_TRUNC`: Write (overwrite).
- `O_WRONLY | O_CREAT | O_APPEND`: Write (append).
- `O_EXCL`: Fail if file exists (useful for atomic file locking).

---

## 2. Synchronization Primitives

### Mutexes
**Directory:** `mutex/`
**Files:** `helper.c` (Example)

Standard POSIX Mutex usage.

- **Static Init**: `pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;`
- **Lock**: `pthread_mutex_lock(&m);`
- **Unlock**: `pthread_mutex_unlock(&m);`

**Best Practice**: Always unlock in the same scope or ensure unlock paths are guaranteed even on error.

### Condition Variables
**Directory:** `condvariables/`
**Files:** `helper.c` (Example)

Use for waiting on a state change (e.g., "Queue is not empty").

| Function | Description |
| :--- | :--- |
| `pthread_cond_wait(&cond, &mutex)` | Releases mutex, sleeps. Re-acquires mutex upon wake. **Must be in a while loop.** |
| `pthread_cond_signal(&cond)` | Wakes **one** waiting thread. |
| `pthread_cond_broadcast(&cond)` | Wakes **all** waiting threads. |

**Pattern**:
```c
pthread_mutex_lock(&m);
while (state != READY) {
    pthread_cond_wait(&cond, &m);
}
// Do work
pthread_mutex_unlock(&m);
```

### Semaphores
**Directory:** `semaphores/`
**Files:** `helper.c` (Example), `toolkit.c` (Dynamic Wrappers)

Useful for counting resources (e.g., "5 slots available").

**Dynamic Toolkit API (`semaphores/toolkit.c`)**:
| Function | Signature | Description |
| :--- | :--- | :--- |
| **Create** | `sem_t* create_dynamic_sem(int val)` | Mallocs and inits a semaphore. Exits on error. |
| **Free** | `void free_dynamic_sem(sem_t *sem)` | Destroys and frees the semaphore. |

**Standard API**:
- `sem_wait(&sem)`: Decrement (Block if 0).
- `sem_post(&sem)`: Increment (Signal).

### Barriers
**Directory:** `barrier/`
**Files:** `helper.c` (Example), `toolkit.c` (Dynamic Wrappers)

Use to force N threads to reach a point before *any* of them proceed.

**Dynamic Toolkit API (`barrier/toolkit.c`)**:
| Function | Signature | Description |
| :--- | :--- | :--- |
| **Create** | `pthread_barrier_t* create_dynamic_barrier(int count)` | Init barrier for `count` threads. |
| **Free** | `void free_dynamic_barrier(pthread_barrier_t *bar)` | Destroys and frees. |

**Key Behavior**:
- `pthread_barrier_wait(&bar)` returns `PTHREAD_BARRIER_SERIAL_THREAD` to **exactly one** thread. Use this if you need initialized cleanup or a "leader" action.

---

## 3. Threading Toolkit

**Directory:** `threads/`
**Files:** `toolkit.c`

Convenience wrappers to handle `malloc` + `pthread_create` boilerplate. Includes error checking macros.

| Function | Signature | Description |
| :--- | :--- | :--- |
| **Single** | `pthread_t* create_single_thread(func, arg)` | Spawns 1 thread. Returns pointer to TID. |
| **Join Single** | `void join_single_thread(pthread_t *tid)` | Joins and frees the TID pointer. |
| **Array** | `pthread_t* create_thread_array(int n, func, arg)` | Spawns N threads. Returns array of TIDs. |
| **Join Array** | `void join_thread_array(pthread_t *tids, int n)` | Joins all N threads and frees array. |

**Note**: In `create_thread_array`, the SAME `arg` is passed to all threads. If you need unique IDs (e.g., Thread 0, Thread 1), pass a struct array or handle indexing carefully (race condition warning if passing address of loop variable).

---

## 4. Signal Handling

**Directory:** `signals/`
**Files:** `helper.c`

Helper pattern for handling `SIGINT` (Ctrl+C) gracefully in multithreaded apps.

**Pattern**:
1. **Block Signals** in `main` *before* creating threads:
   ```c
   sigset_t set;
   sigemptyset(&set);
   sigaddset(&set, SIGINT);
   pthread_sigmask(SIG_BLOCK, &set, NULL);
   ```
2. **Dedicated Thread**: Spawn a specific thread to run `sigwait`.
   - See `signals/helper.c` or `sigblock_cleanup.c`.
3. **Shutdown Flag**: Set a volatile flag when signal is caught to tell other threads to exit cleanly.

---

## 5. Producer-Consumer Core

**File:** `producer_consumer_core.c`

A complete implementation of a thread pool worker logic.

**Struct `thread_pool_t`**:
- `task_t *buffer`: Stack/Queue of tasks.
- `pthread_mutex_t lock`: Protects the structure.
- `pthread_cond_t work_ready`: Workers sleep on this.
- `sem_t empty_slots`: Producer waits on this if buffer is full.

**Worker Routine**:
`void* worker_routine(void *arg)`:
- Loops forever.
- Locks mutex.
- Waits on condition variable (`while count == 0 && !shutdown`).
- Pops task.
- Unlocks (Critical!).
- **Executes task**. 

---

## 6. Compilation Guide

Most of these files use `pthread` and `rt` (for shared memory/semaphores sometimes).

**Standard Build Command**:
```bash
gcc -Wall -Wextra -pthread -o my_program my_program.c -lrt
```

**Including Modules**:
Since these are small helpers, it is often easiest to include the `.c` file directly if you are writing a single-file lab solution (though not "best practice" for large software):
```c
#include "Reusable/threads/toolkit.c"
// Your code
```

Or better, compile as objects:
```bash
gcc -c Reusable/threads/toolkit.c -o toolkit.o
gcc main.c toolkit.o -pthread -o main
```
