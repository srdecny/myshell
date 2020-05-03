#include "utils.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>

int shell_retval = 0;

void *safe_malloc(int size) {
    void *res = malloc(size);
    if (res == NULL) {
        err(12, "malloc failed"); // ENOMEM
    }
    return res;
}

void safe_pipe(int *pipefd) {
    if (pipe(pipefd) == -1) {
        err(EXIT_FAILURE, "pipe failed");
    } 
    return;
}

void safe_dup2(int oldfd, int newfd) {
    if (dup2(oldfd, newfd) == -1) {
        err(EXIT_FAILURE, "dup2 failed");
    }
    return;
}

int safe_open(char *filename, int flags) {
    int fd = open(filename, flags, 0666);
    if (fd == -1) {
        err(EXIT_FAILURE, "open failed: %s", filename);
    }
    return fd;
}

void safe_close(int fd) {
    if (close(fd) == -1) {
        err(EXIT_FAILURE, "close failed");
    }
    return;
}

pid_t safe_fork() {
    pid_t pid = fork();
    if (pid == -1) {
        err(EXIT_FAILURE, "fork failed");
    }
    return pid;
}

void exit_shell() {
    exit(shell_retval);
}

void safe_setenv(char *name, char* value) {
    if (setenv(name, value, 0) == -1) {
        err(EXIT_FAILURE, "setenv failed");
    }
    return;
}