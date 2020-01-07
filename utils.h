#pragma once

void *safe_malloc(int size);
char *concat_array(int count, char *words[]);
void exit_shell();
void handle_sigint_executing(int sig);
void handle_sigint_reading(int sig);

