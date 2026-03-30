#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

int main()
{
    char input[MAX_LINE];
    char *args[MAX_ARGS];

    while (1)
    {
        printf("> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;
        
        if (strcmp(input, "\n") == 0)
            continue;

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "exit") == 0)
            break;
        
        parse_input(input, args);

        pid_t pid = fork();

        if (pid < 0)
            continue;

        if (pid == 0)
        {
            execvp(args[0], args);
            _exit(1);
        }
        else
        {
            int status;
            wait(&status);
        }   
    }
    
    return 0;
}