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
            // Report and skip inaccessible files
            fprintf(stderr, "Cannot open '%s': %s\n", dp->d_name, strerror(errno));
            continue;
        }

        ssize_t bytes_read;
        long int filesize = 0;

        // Try reading file contents
        while ((bytes_read = read(fd, buf, BUF_SIZE)) > 0)
            filesize += bytes_read;

        if (bytes_read < 0)
        {
            if (errno == EISDIR)
            {
                // It's a directory — print notice, skip it
                fprintf(stderr, "Skipping directory '%s'\n", dp->d_name);
            }
            else if (errno == EACCES)
            {
                // Permission denied
                fprintf(stderr, "Access denied reading '%s'\n", dp->d_name);
            }
            else
            {
                // Other read error
                fprintf(stderr, "Error reading '%s': %s\n", dp->d_name, strerror(errno));
            }
        }
        else
        {
            sum += filesize;
        }

        if (close(fd) < 0)
            fprintf(stderr, "Error closing '%s': %s\n", dp->d_name, strerror(errno));
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


    FILE *fptr = fopen("sample.txt", "w"); 
    if (fptr == NULL) 
    { 
        printf("Could not open file"); 
        return 0; 
    } 
    for (int i = 1; i < argc; i += 2)
    {
        if (chdir(argv[i])){
            fprintf(stderr, "Cannot enter directory '%s': %s\n", argv[i], strerror(errno));
            continue;
        }
            

        long int minsize = atol(argv[i + 1]);
        if (scan_dir() > minsize)
            fprintf(fptr,"%s\n", argv[i]);

        if (chdir(path))
            ERR("chdir");
    }
    fclose(fptr);

    return EXIT_SUCCESS;
}
