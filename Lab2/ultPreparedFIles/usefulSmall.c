// Check if File Exists
if (access("filename.txt", F_OK) == 0) {
    // File exists
} else {
    // File does not exist
}

//Get Random Range [Min, Max]
srand(time(NULL) ^ getpid()); // Seed with PID
int r = min + rand() % (max - min + 1);

//Atomic "Printf" to File
char buf[128];
int len = sprintf(buf, "PID %d says hello\n", getpid());
bulk_write(fd, buf, len);


// Unblock Signals (The "Open Ears" Fix)
// Put this at the start of a Child process if the Parent had blocked signals.
sigset_t empty;
sigemptyset(&empty);
sigprocmask(SIG_SETMASK, &empty, NULL);
//


//Full Copy-Paste Snippet
void increment_file_counter(const char *filename) {
    // 1. Open RW. NO O_APPEND! NO O_TRUNC!
    // We need existing data, so don't truncate. We need to overwrite, so don't append.
    int fd = open(filename, O_RDWR);
    if (fd < 0) ERR("open");

    // 2. Read
    char buf[32];
    // Read enough to get the number. 
    int bytes_read = read(fd, buf, sizeof(buf) - 1);
    if (bytes_read < 0) ERR("read");
    buf[bytes_read] = '\0'; // Null terminate

    // 3. Logic
    int val = atoi(buf);
    val++; 
    
    // 4. Rewind
    if (lseek(fd, 0, SEEK_SET) < 0) ERR("lseek");

    // 5. Write back
    int len = sprintf(buf, "%d", val);
    if (write(fd, buf, len) < 0) ERR("write");

    close(fd);
}