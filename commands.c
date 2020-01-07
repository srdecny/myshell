#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include "tailq.c"
#include "utils.h"
#include "shared.h"
#include "commands.h"

void process_command(struct cmd_s *cmd) {
    struct cmd *c = NULL;
    STAILQ_FOREACH(c, cmd, entries) {
        struct node *n = NULL;

        // Count the arguments and create a null-terminated array with pointers to the argumetns
        int argc = 0;
        STAILQ_FOREACH(n, &c->node, entries) {
            argc++;
        }
        char *argv[argc + 1];

        argc = 0;
        STAILQ_FOREACH(n, &c->node, entries) {
            argv[argc] = n->group;
            argc++;
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
            if (fork() == 0) {
                int retval = execvp(argv[0], argv);
                if (retval == -1) {
                    fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
                }
                exit(retval);
            } else {
                wait(&stat);
            }

            if (WIFEXITED(stat)) {
                shell_retval = WEXITSTATUS(stat);
            }
        }
   }
   free_tailq(cmd);
}   

void internal_cd(char *location) {
    char *prev_pwd = getenv("PWD");
    char *new_pwd = NULL;

    int chdir_ret = 1;
    if (location == NULL) {
        char *home_dir = getenv("HOME");
        chdir_ret = chdir(home_dir); // $HOME is set by the system upon logging in and thus is never null
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
        setenv("OLDPWD", prev_pwd, 1);
    } else if (chdir_ret == -1) {
        fprintf(stderr, "cd: %s\n", strerror(errno));
    }

    shell_retval = errno;
}

void exit_shell() {
	exit(shell_retval);
}
