/**
 * @file main.c
 * @author Waylon Walsh
 * @brief Main file for the shell project
 * @date 2023-09-26
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h> 
#include <readline/readline.h>
#include <readline/history.h>
#include "../src/lab.h"

/**
 * @brief Cleanup function to free resources used by readline and history
 * 
 * This function is called before the shell exits to ensure cleanup
 * of readline and history resources.
 * 
 * leveraging the GNU Readline library. 
 */
void cleanup() {
    clear_history();
    rl_clear_history();
    rl_free_line_state();
    rl_cleanup_after_signal();
}

/**
 * @brief Main function of the shell
 * 
 * This function initializes the shell, processes command-line arguments,
 * and runs the main loop of the shell, handling user input and executing commands.
 * 
 * 
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @return int Exit status of the shell
 */

int main(int argc, char *argv[]) {
    struct shell sh = {0};  // Initialize shell structure

    // Parse arguments and exit if version was printed
    if (parse_args(argc, argv)) {
        return 0;
    }

    printf("Starting shell...\n");

    char *line = NULL;
    // for custom prompt
    char *prompt = get_prompt("MY_PROMPT");
    
    sh_init(&sh);  // Initialize shell

    // Initialize readline and history
    rl_initialize();
    using_history();

    // Main shell loop
    while (1) {
        // Check and update status of background jobs
        update_job_status();  
        // Get user input using readline
        line = readline(prompt);
        
        if (line == NULL) {
            printf("\n");
            break;  // EOF (Ctrl+D) detected, exit the shell
        }

        // Process non-empty lines
        if (*line) {
            // Add line to history
            add_history(line);
            // Trim whitespace from the input
            char *trimmed_line = trim_white(line);
            if (strlen(trimmed_line) > 0) {
                // Parse the command line
                char **args = cmd_parse(trimmed_line);
                if (args) {
                    if (!do_builtin(&sh, args)) {
                        // Not a built-in command, execute it as an external command
                        execute_command(args, &sh);
                    }
                    // Free the parsed command
                    cmd_free(args);
                }
            }
        }
        // Free the line buffer
        free(line);
        line = NULL;
    }

    printf("Exiting shell...\n");
    // Cleanup and exit
    free(prompt);
    cleanup();
    sh_destroy(&sh);
    return 0;
}