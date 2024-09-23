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

#define MAX_JOBS 100

struct job {
    int job_id;
    pid_t pid;
    char *command;
    bool is_background;
    bool is_done;
};

struct job jobs[MAX_JOBS];
int next_job_id = 1;

void initialize_jobs() {
    for (int i = 0; i < MAX_JOBS; i++) {
        jobs[i].job_id = 0;
        jobs[i].pid = 0;
        jobs[i].command = NULL;
        jobs[i].is_background = false;
        jobs[i].is_done = false;
    }
}

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

char *get_prompt(const char *env) {
    char* custom_prompt = getenv(env);
    if (custom_prompt != NULL) {
        return strdup(custom_prompt);
    }
    return strdup("shell>");
}

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

void cmd_free(char **tokens) {
    if (tokens == NULL) return;

    // Free the single allocated block
    free(tokens);
}

char *trim_white(char *line) {
    if (line == NULL) return NULL;

    // Trim leading space
    while(isspace((unsigned char)*line)) line++;

    if(*line == 0)  // All spaces?
        return line;

    // Trim trailing space
    char *end = line + strlen(line) - 1;
    while(end > line && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    end[1] = '\0';

    return line;
}

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

void sh_destroy(struct shell *sh) {
    if (sh->prompt) {
        free(sh->prompt);
    }
    clear_history();  // Clear readline history
}

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

void parse_args(int argc, char **argv) {
    // This function can be implemented if you need to parse command-line arguments
    // for the shell itself (not for the commands run within the shell)
    (void)argc;  // Silence unused parameter warning
    (void)argv;  // Silence unused parameter warning
}