//This is a file to get the most useful things about a lab

//This line is to launch the error and the source of it, use it in your code
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

//1.Open and reading a directory
#include <dirent.h>
void open_dir(DIR *tdirp, struct dirent *tdp){

    //opening the directory and placing it in tdir
    if ((tdirp = opendir(".")) == NULL) ERR("opendir");
    //reading contents of the directory one by one
    do
    {
        errno = 0;
        if ((tdp = readdir(tdirp)) != NULL)
        {
            //any code you want
        }
    } while (tdp != NULL);
    if (errno != 0)
        ERR("readdir");
    //Always close the directory for good practice
    if (closedir(tdirp))
        ERR("closedir");
}

//2. using lstat and stat
//difference between them explained in the manpages really simple
#include <fcntl.h>
#include <sys/stat.h>
//after reading a directory and have access to its contents we will use the struct sirent we got
void exo_lstat(struct stat filestat, struct dirent dp){
    if(lstat(dp.d_name, &filestat))ERR("lstat");
    //then we can use things like S_ISDIR to check the type of file and so on
}