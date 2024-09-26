/**
 * @file lab.c
 * @author Waylon Walsh
 * @brief Implementation of a simple shell with job control
 * @date 2023-09-26
 *
 * This file contains the implementation of various functions required
 * for a simple shell, including job control, command parsing, and
 * execution of both built-in and external commands.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <termios.h>
#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <pwd.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <dirent.h>
#include "lab.h"
#include <getopt.h> 

#define MAX_JOBS 100 // Maximum number of jobs that can be managed

/**
 * @brief Structure to represent a job in the shell
 */
struct job {
    int job_id;         // Unique identifier for the job
    pid_t pid;          // Process ID of the job
    char *command;      // Command string of the job
    bool is_background; // Flag to indicate if it's a background job
    bool is_done;       // Flag to indicate if the job is completed
};

struct job jobs[MAX_JOBS];  // Array to store all jobs
int next_job_id = 1;        // Counter for assigning job IDs

/**
 * @brief Initialize the jobs array
 *
 * This function sets up the jobs array with default values.
 */
void initialize_jobs() {
    for (int i = 0; i < MAX_JOBS; i++) {
        jobs[i].job_id = 0;
        jobs[i].pid = 0;
        jobs[i].command = NULL;
        jobs[i].is_background = false;
        jobs[i].is_done = false;
    }
}

/**
 * @brief Add a new job to the jobs array
 *
 * @param pid Process ID of the new job
 * @param command Command string of the job
 * @param is_background Flag indicating if it's a background job
 * @return int Job ID of the newly added job, or -1 if no space available
 */
int add_job(pid_t pid, char *command, bool is_background) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id == 0) {
            jobs[i].job_id = next_job_id++;
            jobs[i].pid = pid;
            jobs[i].command = strdup(command);
            jobs[i].is_background = is_background;
            jobs[i].is_done = false;
            return jobs[i].job_id;
        }
    }
    return -1; // No space for new job
}

/**
 * @brief Remove a job from the jobs array
 *
 * @param job_id ID of the job to be removed
 */
void remove_job(int job_id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id == job_id) {
            free(jobs[i].command);
            jobs[i].job_id = 0;
            jobs[i].pid = 0;
            jobs[i].command = NULL;
            jobs[i].is_background = false;
            jobs[i].is_done = false;
            break;
        }
    }
}

/**
 * @brief Update the status of all jobs
 *
 * This function checks the status of all jobs and updates them accordingly.
 */
void update_job_status() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id != 0 && !jobs[i].is_done) {
            int status;
            pid_t result = waitpid(jobs[i].pid, &status, WNOHANG);
            if (result == jobs[i].pid) {
                jobs[i].is_done = true;
                printf("[%d] Done %s\n", jobs[i].job_id, jobs[i].command);
            }
        }
    }
}

/**
 * @brief Print all jobs
 *
 * This function prints the status of all jobs in the jobs array.
 */
void print_jobs() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id != 0) {
            if (jobs[i].is_done) {
                printf("[%d] Done %s\n", jobs[i].job_id, jobs[i].command);
            } else {
                printf("[%d] %d Running %s\n", jobs[i].job_id, jobs[i].pid, jobs[i].command);
            }
        }
    }
}

/**
 * @brief Get the shell prompt
 *
 * This function returns the shell prompt, either from an environment variable
 * or a default value.
 *
 * @param env Name of the environment variable to check for custom prompt
 * @return char* The shell prompt string
 */
char *get_prompt(const char *env) {
    char* custom_prompt = getenv(env);
    if (custom_prompt != NULL) {
        return strdup(custom_prompt);
    }
    return strdup("shell>");
}

/**
 * @brief Parse a command line into an array of arguments
 *
 * @param line The command line to parse
 * @return char** Array of parsed arguments
 */
char **cmd_parse(const char *line) {
    if (line == NULL) return NULL;

    int bufsize = 64; // Initial buffer size for tokens
    int position = 0;
    char **tokens;

    size_t line_length = strlen(line) + 1;
    size_t total_size = bufsize * sizeof(char*) + line_length * sizeof(char);

    tokens = malloc(total_size);
    if (!tokens) {
        fprintf(stderr, "allocation error\n");
        exit(EXIT_FAILURE);
    }

    // The buffer where the tokens will point into
    char *buffer = (char *)(tokens + bufsize);
    strcpy(buffer, line);

    char *token = strtok(buffer, " \t\r\n\a");
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            // Need to reallocate tokens and adjust buffer pointer
            bufsize += 64;
            total_size = bufsize * sizeof(char*) + line_length * sizeof(char);
            tokens = realloc(tokens, total_size);
            if (!tokens) {
                fprintf(stderr, "allocation error\n");
                exit(EXIT_FAILURE);
            }
            buffer = (char *)(tokens + bufsize);
            // No need to copy buffer again; it's already in place
        }

        token = strtok(NULL, " \t\r\n\a");
    }
    tokens[position] = NULL;

    return tokens;
}

/**
 * @brief Free the memory allocated by cmd_parse
 *
 * @param tokens The array of tokens to free
 */
void cmd_free(char **tokens) {
    if (tokens == NULL) return;

    // Free the single allocated block
    free(tokens);
}

/**
 * @brief Trim whitespace from the beginning and end of a string
 *
 * @param line The string to trim
 * @return char* The trimmed string
 */
char *trim_white(char *line) {
    if (line == NULL) return NULL;

    // Trim leading space
    while(isspace((unsigned char)*line)) line++;

    if(*line == 0)  // will be true if original string was composed entirely of spaces
        return line;

    // Trim trailing space
    char *end = line + strlen(line) - 1;
    while(end > line && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    end[1] = '\0';

    return line;
}


/**
 * @brief Handle built-in shell commands
 *
 * This function checks if a command is built-in and executes it if so.
 *
 * @param sh Pointer to the shell structure
 * @param argv Array of command arguments
 * @return true if the command was a built-in command, false otherwise
 */
bool do_builtin(struct shell *sh, char **argv) {
    if (argv == NULL || argv[0] == NULL) return false;

    if (strcmp(argv[0], "exit") == 0) {
        sh_destroy(sh);
        exit(EXIT_SUCCESS);
    } else if (strcmp(argv[0], "cd") == 0) {
        change_dir(argv);
        return true;
    } else if (strcmp(argv[0], "history") == 0) {
        print_history();
        return true;
    } else if (strcmp(argv[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("getcwd() error");
        }
        return true;
    } else if (strcmp(argv[0], "ls") == 0 && argv[1] == NULL) {
        // Only handle 'ls' without arguments as a built-in command
        DIR *d;
        struct dirent *dir;
        d = opendir(".");
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (dir->d_name[0] != '.') {
                    printf("%s\n", dir->d_name);
                }
            }
            closedir(d);
        } else {
            perror("opendir() error");
        }
        return true;
    } else if (strcmp(argv[0], "jobs") == 0) {
        print_jobs();
        return true;
    }

    return false;  // Not a builtin command
}

/**
 * @brief Execute a command
 *
 * This function forks a new process to execute the given command.
 *
 * @param argv Array of command arguments
 * @param sh Pointer to the shell structure
 * @return int Status of the command execution
 */
int execute_command(char **argv, struct shell *sh) {
    if (argv == NULL || argv[0] == NULL) {
        return 1;
    }

    // Check if it's a background process
    bool is_background = false;
    int i;
    for (i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], "&") == 0 || (strlen(argv[i]) > 0 && argv[i][strlen(argv[i])-1] == '&')) {
            is_background = true;
            if (strcmp(argv[i], "&") == 0) {
                argv[i] = NULL;  // Remove the '&' from the arguments
            } else {
                argv[i][strlen(argv[i])-1] = '\0';  // Remove the '&' from the end of the command
            }
            break;
        }
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        if (!is_background) {
            // Set the process group ID only for foreground processes
            setpgid(0, 0);
            tcsetpgrp(sh->shell_terminal, getpid());
        }
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        
        if (execvp(argv[0], argv) == -1) {
            perror("shell");
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        perror("shell");
    } else {
        // Parent process
        if (!is_background) {
            int status;
            setpgid(pid, pid);
            tcsetpgrp(sh->shell_terminal, pid);
            waitpid(pid, &status, WUNTRACED);
            tcsetpgrp(sh->shell_terminal, sh->shell_pgid);
        } else {
            // Background process
            setpgid(pid, pid);
            char command[1024] = "";
            for (int j = 0; argv[j] != NULL; j++) {
                strcat(command, argv[j]);
                strcat(command, " ");
            }
            strcat(command, "&");
            int job_id = add_job(pid, command, true);
            printf("[%d] %d %s\n", job_id, pid, command);
        }
    }

    return 1;
}

/**
 * @brief Change the current working directory
 *
 * @param argv Array of command arguments (argv[1] is the target directory)
 * @return int 0 on success, -1 on failure
 */
int change_dir(char **argv) {
    char *new_dir;
    
    if (argv[1] == NULL) {
        // No argument provided, change to HOME directory
        new_dir = getenv("HOME");
        if (new_dir == NULL) {
            // If HOME is not set, use getpwuid
            struct passwd *pw = getpwuid(getuid());
            if (pw == NULL) {
                perror("Failed to get home directory");
                return -1;
            }
            new_dir = pw->pw_dir;
        }
    } else {
        new_dir = argv[1];
    }

    if (chdir(new_dir) != 0) {
        perror("cd");
        return -1;
    } else {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("Current directory: %s\n", cwd);
        } else {
            perror("getcwd() error");
            return -1;
        }
    }
    return 0;
}

/**
 * @brief Initialize the shell
 *
 * This function sets up the shell's terminal settings and signal handlers.
 *
 * @param sh Pointer to the shell structure to initialize
 */
void sh_init(struct shell *sh) {
    sh->shell_terminal = STDIN_FILENO;
    sh->shell_is_interactive = isatty(sh->shell_terminal);

    if (sh->shell_is_interactive) {
        while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp()))
            kill(-sh->shell_pgid, SIGTTIN);

        sh->shell_pgid = getpid();
        if (setpgid(sh->shell_pgid, sh->shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group");
            exit(1);
        }

        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);
        tcgetattr(sh->shell_terminal, &sh->shell_tmodes);
    }

    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    initialize_jobs();
}

/**
 * @brief Destroy the shell and free associated resources
 *
 * This function is responsible for cleaning up resources allocated by the shell.
 * It frees the memory allocated for the shell prompt and clears the command history.
 *
 * @param sh Pointer to the shell structure to be destroyed
 */
void sh_destroy(struct shell *sh) {
    if (sh->prompt) {
        free(sh->prompt);
    }
    clear_history();  // Clear readline history
}

/**
 * @brief Print the command history
 *
 * This function prints out the entire command history of the shell session.
 * It uses the readline library's history functions to access and display
 * each command in the history list.
 */
void print_history() {
    HIST_ENTRY **the_history;
    int i;

    the_history = history_list();
    if (the_history) {
        for (i = 0; the_history[i]; i++) {
            printf("%d: %s\n", i + history_base, the_history[i]->line);
        }
    }
}

/**
 * @brief Parse command-line arguments for the shell
 *
 * This function handles the -v command-line argument, which prints
 * the shell version. It uses getopt to parse arguments.
 *
 * @param argc The number of command-line arguments
 * @param argv An array of strings containing the command-line arguments
 * @return bool True if the shell should exit after parsing args, false otherwise
 */
bool parse_args(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) {
            case 'v':
                printf("Shell version %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
                return true;  // Indicate that the shell should exit
            default:
                fprintf(stderr, "Usage: %s [-v]\n", argv[0]);
                exit(1);
        }
    }
    return false;  // Indicate that the shell should continue running
}