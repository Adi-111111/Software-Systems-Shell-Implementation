#include "s3.h"
#include <errno.h>
#include <limits.h>

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[])
{
    strcpy(shell_prompt, "[s3]$ ");
}

///Prints a shell prompt and reads input from the user
void read_command_line(char line[])
{
    char shell_prompt[MAX_PROMPT_LEN];
    construct_shell_prompt(shell_prompt);
    printf("%s", shell_prompt);

    ///See man page of fgets(...)
    if (fgets(line, MAX_LINE, stdin) == NULL)
    {
        perror("fgets failed");
        exit(1);
    }
    ///Remove newline (enter)
    line[strlen(line) - 1] = '\0';
}

void parse_command(char line[], char *args[], int *argsc)
{
    ///Implements simple tokenization (space delimited)
    ///Note: strtok puts '\0' (null) characters within the existing storage, 
    ///to split it into logical cstrings.
    ///There is no dynamic allocation.

    ///See the man page of strtok(...)
    char *token = strtok(line, " ");
    *argsc = 0;
    while (token != NULL && *argsc < MAX_ARGS - 1)
    {
        args[(*argsc)++] = token;
        token = strtok(NULL, " ");
    }
    
    args[*argsc] = NULL; ///args must be null terminated
}

///Launch related functions
void child(char *args[], int argsc)
{
    ///Implement this function:
    execvp(args[ARG_PROGNAME], args); 
    // If execvp returns, it means an error occurred
    perror("execvp failed");
    exit(1); // Exi

    ///Use execvp to load the binary 
    ///of the command specified in args[ARG_PROGNAME].
    ///For reference, see the code in lecture 3.
}

void launch_program(char *args[], int argsc)
{
    ///Implement this function:

    ///fork() a child process.
    ///In the child part of the code,
    ///call child(args, argv)
    ///For reference, see the code in lecture 2.
    ///Handle the 'exit' command;
    ///so that the shell, not the child process,
    ///exits.
    if (argsc == 0) {
    return;
    }
    
    if (argsc > 0 && strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    /// fork() a child process using the pattern from your example.
    pid_t rc = fork();

    if (rc < 0)
    {
        // Fork failed; exit
        perror("fork failed");
        exit(1);
    }
    else if (rc == 0)
    {
        // This is the child process.
        // It will run the new program.
        child(args, argsc);
    }
}

typedef enum { R_IN, R_OUT_TRUNC, R_OUT_APPEND } RedirType;
typedef struct { RedirType type; char *file; } Redir;

void launch_program_with_redirection(char *args[], int argsc) {

    Redir redirs[16];
    int nredirs = 0;

    char *clean_args[MAX_ARGS];
    int cleanc = 0;

    for (int i = 0; i < argsc; ++i) {
        if (strcmp(args[i], "<") == 0) {
            if (i + 1 >= argsc) { 
                fprintf(stderr, "syntax error\n");
                return; 
            }
            if (nredirs < (int)(sizeof redirs / sizeof redirs[0])) 
            {
                redirs[nredirs++] = (Redir){ R_IN, args[i+1] };
            }
            ++i;
            continue;
        }
        if (strcmp(args[i], ">>") == 0) {
            if (i + 1 >= argsc) { 
                fprintf(stderr, "syntax error: '>>' needs a filename\n");
                return; 
            }
            if (nredirs < (int)(sizeof redirs / sizeof redirs[0])) {
                redirs[nredirs++] = (Redir){ R_OUT_APPEND, args[i+1] };
            }
            ++i;
            continue;
        }
        if (strcmp(args[i], ">") == 0) {
            if (i + 1 >= argsc) {
                fprintf(stderr, "syntax error: '>' needs a filename\n");
                return; 
            }
            if (nredirs < (int)(sizeof redirs / sizeof redirs[0])) {
                redirs[nredirs++] = (Redir){ R_OUT_TRUNC, args[i+1] };
            }
            ++i;
            continue;
        }
        if (cleanc < MAX_ARGS - 1) {
            clean_args[cleanc++] = args[i];
        }
    }
    clean_args[cleanc] = NULL;
    if (cleanc == 0) return;

    pid_t rc = fork();
    if (rc < 0) { perror("fork failed"); exit(1); }
    else if (rc == 0) {
        for (int r = 0; r < nredirs; ++r) {
            if (redirs[r].type == R_IN) {
                int fd = open(redirs[r].file, O_RDONLY);
                if (fd < 0) { 
                    perror("open input"); exit(1);
                }
                if (dup2(fd, STDIN_FILENO) < 0) { 
                    perror("dup2 stdin"); exit(1);
                }
                close(fd);
            } else if (redirs[r].type == R_OUT_TRUNC) {
                int fd = open(redirs[r].file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { 
                    perror("open output"); exit(1);
                }
                if (dup2(fd, STDOUT_FILENO) < 0) {
                    perror("dup2 stdout"); exit(1);
                }
                close(fd);
            } else {
                int fd = open(redirs[r].file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (fd < 0) {
                    perror("open output"); exit(1);
                }
                if (dup2(fd, STDOUT_FILENO) < 0) {
                    perror("dup2 stdout"); exit(1);
                }
                close(fd);
            }
        }
        execvp(clean_args[0], clean_args);
        perror("execvp failed");
        exit(1);
    }

}


bool command_with_redirection(const char line[]) {
    for (const char *p = line; *p; ++p) {
        if (*p == '<' || *p == '>') return true;
    }
    return false;
}


bool is_cd(const char *line)
{
    while (*line == ' ' || *line == '\t') line++;
    return (line[0] == 'c' && line[1] == 'd' &&
            (line[2] == '\0' || line[2] == ' ' || line[2] == '\t'));
}

void init_lwd(char *lwd)
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {

        snprintf(lwd, MAX_PROMPT_LEN - 6, "%s", cwd);
    } else {
        lwd[0] = '\0';
    }
}

void run_cd(char *args[], int argsc, char *lwd)
{
    char old[PATH_MAX];
    if (getcwd(old, sizeof(old)) == NULL) {
        perror("getcwd");
        return;
    }

    if (argsc == 1) {
        char *home = getenv("HOME");
        if (!home) {
            fprintf(stderr, "cd: HOME not set\n");
            return;
        }
        if (chdir(home) != 0) {
            perror("cd");
            return;
        }
        snprintf(lwd, MAX_PROMPT_LEN - 6, "%s", old);
        return;
    }

    if (strcmp(args[1], "-") == 0) {
        if (lwd[0] == '\0') {
            fprintf(stderr, "cd: OLDPWD not set\n");
            return;
        }
        if (chdir(lwd) != 0) {
            perror("cd");
            return;
        }
        printf("%s\n", lwd);
        fflush(stdout);
        snprintf(lwd, MAX_PROMPT_LEN - 6, "%s", old);
        return;
    }
    if (args[1][0] == '~') {
        char path[PATH_MAX];
        char *home = getenv("HOME");

        if (!home) {
            fprintf(stderr, "cd: HOME not set\n");
            return;
        }
        if (args[1][1] == '\0') {
            snprintf(path, sizeof(path), "%s", home);
        }
        else if (args[1][1] == '/') {
            snprintf(path, sizeof(path), "%s%s", home, args[1] + 1);
        } else {
            snprintf(path, sizeof(path), "%s", args[1]);
        }

        if (chdir(path) != 0) {
            perror("cd");
            return;
        }
        snprintf(lwd, MAX_PROMPT_LEN - 6, "%s", old);
        return;
    }
    if (chdir(args[1]) != 0) {
        perror("cd");
        return;
    }
    snprintf(lwd, MAX_PROMPT_LEN - 6, "%s", old);
}