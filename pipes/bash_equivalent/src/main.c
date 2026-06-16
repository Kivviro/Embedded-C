#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_LINE 1024
#define MAX_ARGS 64

void parse_input(char *input, char **args)
{
    int i = 0;

    args[i] = strtok(input, " \n");
    while(args[i] !=  NULL && i < MAX_ARGS - 1)
    {
        i++;
        args[i] = strtok(NULL, " \n");
    }
    args[i] = NULL;
}

char **parse_pipeline(char *input, int *count)
{
    int capacity = 4;
    char **commands = malloc(capacity * sizeof(char*));

    int i = 0;
    char *token = strtok(input, "|");

    while (token != NULL)
    {
        if (i >= capacity)
        {
            capacity *= 2;
            commands = realloc(commands, capacity * sizeof(char*));
        }

        commands[i++] = token;
        token = strtok(NULL, "|");
    }

    *count = i;
    return commands;
}

void parse_redirection(char **args, char **infile, char **outfile, 
    int *append)
{
    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], "<") == 0)
        {
            *infile = args[i + 1];
            args[i] = NULL;
        }
        else if (strcmp(args[i], ">") == 0)
        {
            *outfile = args[i + 1];
            *append = 0;
            args[i] = NULL;
        }
        else if (strcmp(args[i], ">>") == 0)
        {
            *outfile = args[i + 1];
            *append = 1;
            args[i] = NULL;
        }
    }
}

int execute_pipeline(char *commands[], int n)
{
    int prev_fd = -1;

    for (int i = 0; i < n; i++)
    {
        int fd[2];

        if (i < n - 1)
            pipe(fd);

        pid_t pid = fork();

        if (pid == 0)
        {
            if (prev_fd != -1)
            {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }
            
            if (i < n - 1)
            {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
            }

            char *args[MAX_ARGS];
            parse_input(commands[i], args);

            char *infile = NULL;
            char *outfile = NULL;
            int append = 0;

            parse_redirection(args, &infile, &outfile, &append);

            if (infile)
            {
                int fd_in = open(infile, O_RDONLY);
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }

            if (outfile)
            {
                int fd_out;
                if (append)
                    fd_out = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                else
                    fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }
            
            execvp(args[0], args);
            _exit(1);
        }
        
        if (prev_fd != -1)
            close(prev_fd);
        
        if (i < n - 1)
        {
            close(fd[1]);
            prev_fd = fd[0];
        }
        
    }
    
    for (int i = 0; i < n; i++)
    {
        wait(NULL);
    }
    
    return 0;
}

int main()
{
    char input[MAX_LINE];

    while (1)
    {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin))
            break;
        
        if (strcmp(input, "\n") == 0)
            continue;

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "exit") == 0)
            break;
        
        int n; 
        char **commands = parse_pipeline(input, &n);
        
        execute_pipeline(commands, n); 
        
        free(commands);
    }
    
    return 0;
}