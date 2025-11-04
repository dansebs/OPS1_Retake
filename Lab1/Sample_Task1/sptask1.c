#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_PATH 101


void scan_dir()
{
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;
    long int sum = 0;
    if ((dirp = opendir(".")) == NULL)
        ERR("opendir");
    do
    {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL)
        {
            if ((dp = readdir(dirp)) != NULL)
        {
            if (lstat(dp->d_name, &filestat))
                ERR("lstat");
            
            sum += filestat.st_size;
            printf("%s %ld\n",dp->d_name,filestat.st_size);
        }
        }
    } while (dp != NULL);
    printf("%ld\n", sum);
    if (errno != 0)
        ERR("readdir");
    if (closedir(dirp))
        ERR("closedir");

}

int main(int argc, char **argv)
{
    char path[MAX_PATH];
    if(getcwd(path, MAX_PATH) == NULL) ERR("getcwd");

    scan_dir();
    return EXIT_SUCCESS;
}