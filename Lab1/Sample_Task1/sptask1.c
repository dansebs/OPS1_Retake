#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_PATH 101


long int scan_dir()
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
        }
        }
    } while (dp != NULL);
    
    if (errno != 0)
        ERR("readdir");
    if (closedir(dirp))
        ERR("closedir");

    return sum;
}

int main(int argc, char **argv)
{
    char path[MAX_PATH];
    if(getcwd(path, MAX_PATH) == NULL) ERR("getcwd");

    for (int i = 1; i < argc; i+=2)
    {
        if (chdir(argv[i]))
        {
            ERR("chdir");
        }
        
        long int minsize = atoi(argv[i+1]);
        if(scan_dir() > minsize) printf("%s\n",argv[i]);
        if(chdir(path))
            ERR("chdir");
    }
    return EXIT_SUCCESS;
}