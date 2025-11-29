#ifndef MYLIB_H
#define MYLIB_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

// --- MACROS ---
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

// --- GLOBAL VARIABLES (from sigInfoHandler.c) ---
extern volatile sig_atomic_t last_sender_pid;
extern volatile sig_atomic_t last_signal_code;

// --- PROTOTYPES ---

// From bulkReadWrite.c
ssize_t bulk_read(int fd, char *buf, size_t count);
ssize_t bulk_write(int fd, char *buf, size_t count);

// From readLine.c
ssize_t read_line(int fd, char *buf, size_t size);

// From handler.c / sigInfoHandler.c
void set_handler(void (*f)(int), int sigNo);
void set_siginfo_handler(int sigNo);

#endif