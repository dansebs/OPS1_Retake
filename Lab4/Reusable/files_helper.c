#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

int main() {
    int fd;

    // 1. OPEN FOR READING ONLY
    // Fails if the file does not exist.
    fd = open("test.txt", O_RDONLY);
    if (fd == -1) perror("Error O_RDONLY");
    close(fd);

    // 2. OPEN FOR WRITING ONLY (CREATE & TRUNCATE)
    // Creates if missing. Wipes content if it exists.
    // 0644 = Owner(rw-), Group(r--), Others(r--)
    fd = open("test_write.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) perror("Error O_WRONLY");
    close(fd);

    // 3. OPEN FOR APPENDING
    // Creates if missing. New data is always added to the end.
    fd = open("test_append.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) perror("Error O_APPEND");
    close(fd);

    // 4. OPEN FOR BOTH READING AND WRITING
    // Does not truncate by default. 
    fd = open("test_both.txt", O_RDWR | O_CREAT, 0644);
    if (fd == -1) perror("Error O_RDWR");
    close(fd);

    // 5. EXCLUSIVE CREATE (FAIL IF FILE EXISTS)
    // Useful for locking or ensuring you don't overwrite a file.
    fd = open("unique_file.txt", O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd == -1) {
        printf("File already exists, so O_EXCL failed as expected.\n");
    }
    close(fd);

    return 0;
}