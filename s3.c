#include "s3.h"

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

void launch_program_with_redirection(char *args[], int argsc){
    char *clean_args[MAX_ARGS];
    int cleanc = 0;
    char *infile = NULL;
    char *outfile = NULL;
    int append = 0;

    for (int i= 0; i<argsc; ++i){
        if(strcmp(args[i], "<") == 0){
            if(i + 1>= argsc) {
                fprintf(stderr, "syntax error");
                return;
            }
            infile = args[i+1];
            ++i;
            continue;
        }
        if(strcmp(args[i], ">>") == 0){
            if (i+1 >= argsc){
                fprintf(stderr, "syntax error");
                return;
            }
            outfile = args[i+1];
            append = 1;
            ++i;
            continue;
        }
        if (strcmp(args[i], ">") == 0) {
            if (i +1 >= argsc){
                fprintf(stderr, "syntax error");
                return;
            }
            outfile = args[++i];
            append = 0;
            continue;
        }
        if (cleanc < MAX_ARGS - 1){
            clean_args[cleanc++] = args[i];
        }
    }

    clean_args[cleanc] = NULL;
    if (cleanc == 0){
        return;
    }

    pid_t rc = fork();
    if (rc < 0){
        perror("fork failed");
        exit(1);
    }
    else if (rc == 0) {
        if (infile) {
            int fd_in = open(infile, O_RDONLY);
            if(fd_in < 0) {
                perror("error input");
                exit(1);
            }
            if(dup2(fd_in, STDIN_FILENO) < 0){
                perror("dup2 stdin");
                exit(1);
            }
            close(fd_in);
        }

        if(outfile){
            int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
            int fd_out = open(outfile, flags, 0644);
            if (fd_out < 0){
                perror("output open");
                exit(1);
            }
            if (dup2(fd_out, STDOUT_FILENO) < 0){
                perror("dup2 out");
                exit(1);
            }
            close(fd_out);
        }

        execvp(clean_args[0], clean_args);
        perror("execvp failed");
        exit(1);
    }


}

bool command_with_redirection(const char line[]){
    char *args[MAX_ARGS];
    int argsc;
    parse_command(line, args, &argsc);
    launch_program_with_redirection(args, argsc);
    reap();

}

