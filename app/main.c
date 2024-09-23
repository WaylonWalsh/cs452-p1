#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "../src/lab.h"

void cleanup() {
    clear_history();
    rl_clear_history();
    rl_free_line_state();
    rl_cleanup_after_signal();
}

int main(int argc, char *argv[]) {
    int opt;
    struct shell sh = {0};  // Initialize shell structure

    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) {
            case 'v':
                printf("Shell version %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
                return 0;
            default:
                fprintf(stderr, "Usage: %s [-v]\n", argv[0]);
                return 1;
        }
    }

    printf("Starting shell...\n");

    char *line = NULL;
    char *prompt = get_prompt("MY_PROMPT");
    
    sh_init(&sh);  // Initialize shell

    rl_initialize();
    using_history();

    while (1) {
        update_job_status();  // Check and update status of background jobs
        line = readline(prompt);
        
        if (line == NULL) {
            printf("\n");
            break;  // EOF (Ctrl+D) detected, exit the shell
        }

        if (*line) {
            add_history(line);
            char *trimmed_line = trim_white(line);
            if (strlen(trimmed_line) > 0) {
                char **args = cmd_parse(trimmed_line);
                if (args) {
                    if (!do_builtin(&sh, args)) {
                        // Not a built-in command, execute it as an external command
                        execute_command(args, &sh);
                    }
                    cmd_free(args);
                }
            }
        }
        
        free(line);
        line = NULL;
    }

    printf("Exiting shell...\n");
    free(prompt);
    cleanup();
    sh_destroy(&sh);
    return 0;
}