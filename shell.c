#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "utils.h"

/* declarations */
int parse_string(const char* in);
int parse_file(FILE *file);

/* set to 1 if received SIGINT during execution */
int sigint_received = 0;

void handle_sigint_executing(int sig) {
    shell_retval = 128 + sig;
    sigint_received = 1;
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
            
            if (sigint_received) {
                fprintf(stderr, "Killed by signal 2.\n");
                sigint_received = 0;
            }

            act.sa_handler = handle_sigint_reading;
            sigaction(SIGINT, &act, NULL);
            char *pwd = getenv("PWD");
            // PWD not set, most likely a login shell
            // Proper behavior would be setting $PWD according to /etc/passwd
            // But that's out of the scope of this simple shell
            if (pwd == NULL) {
                pwd = "";
            }
            int prompt_size = strlen(pwd) + 8;
            char *prompt = safe_malloc(prompt_size);
            snprintf(prompt, prompt_size, "mysh:%s$ ", pwd);
            char *input = readline(prompt);
            free(prompt);

            if (input == NULL) {
                free(input);
                printf("\n");
                exit_shell(); // EOF detected
            }

            add_history(input);
            // Hack, had trouble with EOFs, appending EOL makes the parsing work
            int newlined_input_size = strlen(input) + strlen("\n") + 1;
            char *newlined = safe_malloc(newlined_input_size);
            memset(newlined, 0, newlined_input_size);
            snprintf(newlined, newlined_input_size, "%s%s", input, "\n");
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
            int input_string_size = strlen(argv[2]) + 2;
            char *input_string = safe_malloc(input_string_size);
            snprintf(input_string, input_string_size, "%s%s", argv[2], "\n");
            parse_string(input_string);
            free(input_string);
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
