#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

void *safe_malloc(int size);
void safe_pipe(int *pipefd);
void safe_dup2(int oldfd, int newfd);
int safe_open(char *filename, int flags);
void safe_close(int fd);
pid_t safe_fork();
void safe_setenv(char *name, char *value);

void exit_shell();
extern int shell_retval;

#endif