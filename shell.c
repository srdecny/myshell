#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include "utils.h"

/* declarations */
int parse_string(const char* in);
int parse_file(FILE *file);

void handle_sigint_executing(int sig) {
    shell_retval = 128 + sig;
    fprintf(stderr, "Killed by signal %d.\n", sig);
}

void handle_sigint_reading(int sig) {
    (void)sig;
    printf("\n"); // Move to a new line
    rl_on_new_line(); // Regenerate the prompt on a newline
    rl_replace_line("", 0); // Clear the previous text
    rl_redisplay();
}

int main(int argc, char *argv[]) {

    struct sigaction act;
    memset(&act, 0, sizeof(act));\
    act.sa_handler = handle_sigint_executing;
    sigaction(SIGINT, &act, NULL);
    if (argc == 1) { // Interactive mode
        for (;;) {
            act.sa_handler = handle_sigint_reading;
            sigaction(SIGINT, &act, NULL);
            char *pwd = getenv("PWD");
            char *prompt = safe_malloc(strlen(pwd) + 8);
            sprintf(prompt, "mysh:%s$ ", pwd);
            char *input = readline(prompt);
            free(prompt);

            if (input == NULL) {
                free(input);
                printf("\n");
                exit_shell(); // EOF detected
            }

            add_history(input);
            // Hack, had trouble with EOFs, appending EOL makes the parsing work
            char *newlined = safe_malloc(strlen(input) + 2);
            strcpy(newlined, input);
            strcat(newlined, "\n");
            free(input);
            act.sa_handler = handle_sigint_executing;
            sigaction(SIGINT, &act, NULL);
            parse_string(newlined);	
            free(newlined);
        }
        exit_shell();
    }

    if (argc > 1) {
        if (strcmp("-c", argv[1]) == 0) { // From arguments
            if (argc == 2) {
                fprintf(stderr, "%s: -c option requires an argument", argv[0]);
                exit_shell();
            }
            // Strip the program name and -c flag, add newline 
            char *string = safe_malloc(strlen(argv[2]) + 2);
            strcat(string, argv[2]);
            strcat(string, "\n");
            parse_string(string);
            free(string);
        } else {
            FILE *file = fopen(argv[1], "r");
            if (file == NULL) {
                printf("%s: %s: %s\n", argv[0], argv[1], strerror(errno));
            } else {
                parse_file(file);
                fclose(file);
            }
        }
        exit_shell();
    }
}
