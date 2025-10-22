#include <stdio.h>
#include <stdlib.h>
#include "hello.h"
#include <string.h>
#include <errno.h>

#define ERR(source) (perror(source),\
		     fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),\
		     exit(EXIT_FAILURE))

#define MAXL 20

int hello(void){
	char name[MAXL + 2];
	while (fgets(name, MAXL + 2, stdin) != NULL)
		print("Hello %s", name);

	return EXIT_SUCCESS;
}
