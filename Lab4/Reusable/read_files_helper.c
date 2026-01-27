#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 1024

// 1. READ THE FIRST LINE
void read_first_line(int fd) {
    lseek(fd, 0, SEEK_SET); // Reset to start
    char c;
    printf("First Line: ");
    while (read(fd, &c, 1) > 0 && c != '\n') {
        putchar(c);
    }
    putchar('\n');
}

// 2. READ THE N-th LINE (1-indexed)
void read_nth_line(int fd, int n) {
    lseek(fd, 0, SEEK_SET);
    int current_line = 1;
    char c;
    
    // Skip lines until we reach n
    while (current_line < n && read(fd, &c, 1) > 0) {
        if (c == '\n') current_line++;
    }

    if (current_line == n) {
        printf("Line %d: ", n);
        while (read(fd, &c, 1) > 0 && c != '\n') {
            putchar(c);
        }
        putchar('\n');
    } else {
        printf("Line %d not found.\n", n);
    }
}

// 3. READ THE LAST LINE
void read_last_line(int fd) {
    off_t pos = lseek(fd, -2, SEEK_END); // Start before the final newline
    char c;
    
    // Move backwards until we find the previous newline or start of file
    while (pos > 0) {
        read(fd, &c, 1);
        if (c == '\n') break;
        pos = lseek(fd, -2, SEEK_CUR); // Move back 2: one for read, one to go back
    }
    
    // Now read forward from here to the end
    while (read(fd, &c, 1) > 0 && c != '\n') {
        putchar(c);
    }
    putchar('\n');
}

// 4. FIND LINE BY IDENTIFIER (e.g., "ID:123")
void get_line_by_id(int fd, const char *id) {
    lseek(fd, 0, SEEK_SET);
    char buffer[MAX_LINE];
    int i = 0;
    char c;

    while (read(fd, &c, 1) > 0) {
        if (c != '\n' && i < MAX_LINE - 1) {
            buffer[i++] = c;
        } else {
            buffer[i] = '\0';
            if (strstr(buffer, id) != NULL) {
                printf("Found Match: %s\n", buffer);
                return;
            }
            i = 0;
        }
    }
}

int main() {
    int fd = open("data.txt", O_RDONLY);
    if (fd == -1) {
        perror("Create data.txt first");
        return 1;
    }

    read_first_line(fd);
    read_nth_line(fd, 3);
    read_last_line(fd);
    get_line_by_id(fd, "ERROR_404");

    close(fd);
    return 0;
}