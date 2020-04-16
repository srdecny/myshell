#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "datastructs.c"
#include "utils.h"
#include "commands.h"

#define WRITE_END 1
#define READ_END 0

// See the "Alternate Architecture" section of the following link:
// http://web.cse.ohio-state.edu/~mamrak.1/CIS762/pipes_lab_notes.html
void process_line(struct line_s *line) {
    struct sequence *seq = NULL;
    STAILQ_FOREACH(seq, line, entries) {
        int length = 0;
        struct pipe_segment *seg = NULL;
        // How many pipe segments there are in the pipeline
        STAILQ_FOREACH(seg, &seq->pipeline, entries) {
            length++;
        }

        // No pipelines, no forking (to make cd and exit work)
        if (length == 1) {
            seg = STAILQ_FIRST(&seq->pipeline);
            process_command(&seg->command);
        } else { // Pipelines present
            int stat;
            // Forking so the parent can wait for the pipeline to finish
            if (safe_fork() == 0) { 
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
                            exit_shell();
                        } 
                    // Last command isn't forked so parent can wait on it
                    } else if (index == length - 1) {
                        safe_dup2(pipefd[READ_END], STDIN_FILENO);
                        safe_close(pipefd[READ_END]);
                        safe_close(pipefd[WRITE_END]);
                        process_command(&seg->command);
                        // Wait for all other processes to finish
                        while (wait(&stat) > 0) { ; } 
                        exit_shell();
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
                            exit_shell();
                        } else {
                            safe_close(pipefd[WRITE_END]);
                            safe_close(pipefd[READ_END]);
                            pipefd[0] = new_pipefd[0];
                            pipefd[1] = new_pipefd[1];
                        }
                    }
                    index++;
                }

            } else { // Wait for the sequence to finish.
                wait(&stat);
            }
        }
    }
}
void process_command(struct command_s *cmd) {
    // Count the arguments and create a null-terminated array
    // with pointers to the argumetns
    int argc = 0;
    struct param *p = NULL;

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
        // Execute the command in a child process and wait for it to exit
        int stat = 0;
        pid_t pid = safe_fork();
        if (pid == 0) {
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
            shell_retval = 127; // Unknown command
            exit_shell();
        } else {
            // Have to wait for the fork with exec, because process executing 
            // this might have another children.
            waitpid(pid, &stat, 0);
        }

        if (WIFEXITED(stat)) {
            shell_retval = WEXITSTATUS(stat);
        }
    }
}   

void internal_cd(char *location) {
    char *prev_pwd = getenv("PWD");
    char *new_pwd = NULL;

    int chdir_ret = 1;
    if (location == NULL) {
        char *home_dir = getenv("HOME");
        // $HOME is set by the system upon logging in and thus is never null
        chdir_ret = chdir(home_dir); 
        new_pwd = home_dir;
    } else if (strcmp(location, "-") == 0) {
        char *old_pwd = getenv("OLDPWD");
        if (old_pwd == NULL) {
            fprintf(stderr, "cd: OLDPWD not set\n");
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
    } else if (chdir_ret == -1) {
        fprintf(stderr, "cd: %s\n", strerror(errno));
        shell_retval = errno;
    }

}

