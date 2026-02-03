# Lab 1: Filesystem Operations

## ⚔️ Low-Level (POSIX) vs. High-Level (Standard I/O)

| Feature | Low-Level (`<unistd.h>`, `<fcntl.h>`) | High-Level (`<stdio.h>`) |
| :--- | :--- | :--- |
| **Identifier** | File Descriptor (`int fd`) | File Stream (`FILE *fp`) |
| **Buffering** | **Unbuffered** (Direct syscalls) | **Buffered** (User-space buffer) |
| **Functions** | `open`, `read`, `write`, `close` | `fopen`, `fread`, `fwrite`, `fclose` |
| **Best For** | Devices, sockets, precise control, atomicity. | Text files, general processing, performance. |

---

## 💡 Key Concepts & Questions

### 1. `read` vs. `fread`
* **`read(fd, buf, size)`:** A raw system call. It reads data directly from the kernel into your buffer. It stops reading if it is interrupted by a signal or if the pipe/socket is empty temporarily. It does **not** add a null terminator.
* **`fread(buf, size, count, fp)`:** A wrapper function. It manages a buffer in user memory to reduce expensive system calls. It automatically handles loops to fetch the requested amount of data (mostly).

### 2. `write` vs. `fwrite`
* **`write`:** Pushes data immediately to the kernel. If you write 1 byte 1000 times, you trigger 1000 system calls (slow).
* **`fwrite`:** Writes to a hidden internal buffer. The data is only sent to the kernel (flushed) when the buffer is full, `fflush` is called, or a newline `\n` is encountered (in line-buffered mode). This is significantly faster for many small writes.

### 3. The Necessity of `bulk_read` and `bulk_write`
In the lab, you cannot rely on a single call to `read` or `write` because:
1.  **Partial Operations:** System calls may read/write fewer bytes than requested (e.g., if a disk operation is split). You must loop until `count` becomes 0.
2.  **Interruptions (`EINTR`):** If a signal (like a timer or `Ctrl+C`) occurs during a `read`, the call may fail with `errno = EINTR`.
    
**Solution:** The `bulk_` functions wrap the syscalls in a `do-while` loop and use the macro `TEMP_FAILURE_RETRY` to auto-restart calls interrupted by signals.

---

## 🛠️ Implementation Snippets

### Create/Replace File
**Low-Level:**
```c
// O_WRONLY: Write only
// O_CREAT: Create if missing
// O_TRUNC: Delete content if exists
int fd = open("file.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
High-Level:

```C
// "w+": Write/Update mode (truncates to zero length automatically)
FILE *fp = fopen("file.txt", "w+");
```
Change Permissions
Low-Level (fchmod): Works on the open file descriptor.

```C
fchmod(fd, 0755); // Change to rwxr-xr-x
```
High-Level (chmod): Works on the file path (Standard C does not expose permissions via FILE*).

```C
chmod("path/to/file.txt", 0755);
```
Vectorized Operations (Advanced)
If you need to write data from multiple different variables into a single file contiguously, use writev:
```C
struct iovec iov[2];
iov[0].iov_base = header_struct;
iov[0].iov_len = sizeof(header);
iov[1].iov_base = data_buffer;
iov[1].iov_len = data_len;
writev(fd, iov, 2);
```
---

### 📝 Next Steps
This suite covers standard file manipulation. Lab 1 also briefly mentions **Recursive Directory Scanning** (`nftw`).