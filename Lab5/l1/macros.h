#ifndef MACROS_H
#define MACROS_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

/* The standard error macro from the lab */
#define ERR(source) (perror(source), \
                     fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
                     exit(EXIT_FAILURE))

/* Common usage helper */
void usage(char *pname, char *msg) {
    fprintf(stderr, "USAGE: %s %s\n", pname, msg);
    exit(EXIT_FAILURE);
}

#endif