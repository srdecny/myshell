#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"
#include "commands.h"

#define WRITE_END 1
#define READ_END 0

void process_line(line_t *line) {
    sequence_t *seq = NULL;
    STAILQ_FOREACH(seq, line, entries) {
        int length = 0;
        pipe_segment_t *seg = NULL;
        // How many pipe segments there are in the pipeline
        STAILQ_FOREACH(seg, &seq->pipeline, entries) {
            length++;
        }

        // No pipelines, no forking (to make cd and exit work)
        if (length == 1) {
            seg = STAILQ_FIRST(&seq->pipeline);
            // Find first string (the command) in the segment
            param_t *par = NULL;
            char *str = NULL;
            STAILQ_FOREACH(par, &seg->command, entries) {
               if (par->string != NULL) {
                   str = par->string;
                   break;
               } 
            }
            // Check if the command is a shell builtin
            // If yes, then we don't fork to make cd and exit work properly
            if ((str != NULL) && (
                (strncmp(str, "cd", 2) == 0) ||
                (strncmp(str, "exit", 4) == 0))) {
                process_command(&seg->command);
            } else {
                int stat;
                if (safe_fork() == 0) {
                    process_command(&seg->command);
                } else {
                    wait(&stat);
                    shell_retval = WEXITSTATUS(stat);
                }
            }
        } else { // Pipelines present
            int stat;
            pid_t last_command_pid; // Last command in sequence
            int index = 0; // Which segment is currently processed
            int pipefd[2]; // Currently active pipeline
            STAILQ_FOREACH(seg, &seq->pipeline, entries) {
                if (index == 0) { // First command in the pipeline
                    safe_pipe(pipefd);
                    if (safe_fork() == 0) {
                        safe_dup2(pipefd[WRITE_END], STDOUT_FILENO);
                        safe_close(pipefd[READ_END]);
                        safe_close(pipefd[WRITE_END]);
                        process_command(&seg->command);
                    } 
                // Last command 
                } else if (index == length - 1) {
                    last_command_pid = safe_fork(); // the retval will be saved
                    if (last_command_pid == 0) {
                        safe_dup2(pipefd[READ_END], STDIN_FILENO);
                        safe_close(pipefd[READ_END]);
                        safe_close(pipefd[WRITE_END]);
                        process_command(&seg->command);
                    } else {
                        safe_close(pipefd[READ_END]);
                        safe_close(pipefd[WRITE_END]);
                    }
                } else { // In the middle of the pipeline
                    int new_pipefd[2]; // Create new pipe to be used later
                    safe_pipe(new_pipefd);
                    if (safe_fork() == 0) {
                        safe_dup2(pipefd[READ_END], STDIN_FILENO);
                        safe_dup2(new_pipefd[WRITE_END], STDOUT_FILENO);
                        safe_close(pipefd[READ_END]);
                        safe_close(pipefd[WRITE_END]);
                        safe_close(new_pipefd[READ_END]);
                        safe_close(new_pipefd[WRITE_END]);
                        process_command(&seg->command);
                    } else {
                        safe_close(pipefd[WRITE_END]);
                        safe_close(pipefd[READ_END]);
                        pipefd[0] = new_pipefd[0];
                        pipefd[1] = new_pipefd[1];
                    }
                }
                index++;
            }
            // Wait for all children to finish and save the retval of last child
            pid_t finished_child_pid;
            while((finished_child_pid = wait(&stat)) > 0) {
                if (finished_child_pid == last_command_pid) { 
                    shell_retval = WEXITSTATUS(stat);
                }
            }
        }
    }
}

void process_command(command_t *cmd) {
    // Count the arguments and create a null-terminated array
    // with pointers to the argumetns
    int argc = 0;
    param_t *p = NULL;

    // Redirections
    char *output_file = NULL;
    char *output_append_file = NULL;
    char *input_file = NULL;

    STAILQ_FOREACH(p, cmd, entries) {
        if (p->string != NULL) {
            argc++;
        } else if (p->redirection != NULL) {
            switch (p->redirection->type) {
                case OUTPUT:
                    output_file = p->redirection->file;
                    output_append_file = NULL;
                    break;
                case OUTPUT_APPEND:
                    output_append_file = p->redirection->file;
                    output_file = NULL;
                    break;
                case INPUT:
                    input_file = p->redirection->file;
                    break;
            }
        }
    }

    // Copy arguments into separate array for exec
    char *argv[argc + 1];
    argc = 0;
    STAILQ_FOREACH(p, cmd, entries) {
        if (p->string != NULL) {
            argv[argc] = p->string;
            argc++;
        }
    }
    argv[argc] = NULL;
    if (strcmp(argv[0], "exit") == 0) {
        exit_shell();
    } else if (strcmp(argv[0], "cd") == 0) {
        if (argc > 2) {
            fprintf(stderr, "cd: too many arguments (%d) supplied\n", argc - 1);
            shell_retval = 7;
        } else {
            internal_cd(argv[1]);
        }
    } else {
        // Create redirection files and redirect stdin/stdout to them
        if (output_file != NULL) {
            int fd = safe_open(output_file, O_TRUNC | O_WRONLY | O_CREAT);
            safe_dup2(fd, STDOUT_FILENO);
            safe_close(fd);
        }

        if (output_append_file != NULL) {
            int fd = safe_open(output_append_file,
                    O_APPEND | O_WRONLY | O_CREAT);
            safe_dup2(fd, STDOUT_FILENO);
            safe_close(fd);
        }
        if (input_file != NULL) {
            int fd = safe_open(input_file, O_RDONLY);
            safe_dup2(fd, STDIN_FILENO);
            safe_close(fd);
        }
        int retval = execvp(argv[0], argv);
        if (retval == -1) {
            fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
        }
        exit(127); // Unknown command
    }
}   

void internal_cd(char *location) {
    char *prev_pwd = getenv("PWD");
    char *new_pwd = NULL;

    int chdir_ret = 1;
    if (location == NULL) {
        char *home_dir = getenv("HOME");
        if (home_dir == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            shell_retval = 1;
        } else {
            chdir_ret = chdir(home_dir); 
            new_pwd = home_dir;
        }
    } else if (strcmp(location, "-") == 0) {
        char *old_pwd = getenv("OLDPWD");
        if (old_pwd == NULL) {
            fprintf(stderr, "cd: OLDPWD not set\n");
            shell_retval = 1;
        } else {
            printf("%s\n", old_pwd);
            chdir_ret = chdir(old_pwd);
            new_pwd = old_pwd;
        }
    } else {
        chdir_ret = chdir(location);
        new_pwd = location;
    }

    if (chdir_ret == 0) { // Succesfully changed the directory
        setenv("PWD", new_pwd, 1);
        if (prev_pwd != NULL) {
          setenv("OLDPWD", prev_pwd, 1);
        }
        shell_retval = 0;
    } else if (chdir_ret == -1) {
        fprintf(stderr, "cd: %s\n", strerror(errno));
        shell_retval = errno;
    }
}

