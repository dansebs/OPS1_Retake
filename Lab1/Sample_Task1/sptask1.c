#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_PATH 101
#define BUF_SIZE 4096

long int scan_dir()
{
    DIR *dirp;
    struct dirent *dp;
    char buf[BUF_SIZE];
    long int sum = 0;

    if ((dirp = opendir(".")) == NULL)
        ERR("opendir");

    while ((dp = readdir(dirp)) != NULL)
    {
        // Skip "." and ".."
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            continue;

        int fd = open(dp->d_name, O_RDONLY);
        if (fd < 0)
        {
            // unreadable file, skip
            continue;
        }

        ssize_t bytes_read;
        long int filesize = 0;

        // Try reading file
        while ((bytes_read = read(fd, buf, BUF_SIZE)) > 0)
            filesize += bytes_read;

        if (bytes_read < 0)
        {
            if (errno == EISDIR) {
                // It's a directory — skip gracefully
                close(fd);
                continue;
            } else {
                // Other read errors should still terminate
                ERR("read");
            }
        }

        sum += filesize;

        if (close(fd) < 0)
            ERR("close");
    }

    if (closedir(dirp) < 0)
        ERR("closedir");

    return sum;
}

int main(int argc, char **argv)
{
    char path[MAX_PATH];
    if (getcwd(path, MAX_PATH) == NULL)
        ERR("getcwd");

    for (int i = 1; i < argc; i += 2)
    {
        if (chdir(argv[i]))
            ERR("chdir");

        long int minsize = atol(argv[i + 1]);
        if (scan_dir() > minsize)
            printf("%s\n", argv[i]);

        if (chdir(path))
            ERR("chdir");
    }

    return EXIT_SUCCESS;
}
