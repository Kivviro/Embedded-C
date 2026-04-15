#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int choice;

    if (argc != 2)
        return 1;

    int pid = atoi(argv[1]);

    while (1)
    {
        printf("choice signal:\n");
        printf("1. SIGINT\n");
        printf("2. SIGUSR1\n");
        printf("0. exit sender\n");

        scanf("%d", &choice);

        if (choice == 0)
            return 0;

        switch (choice)
        {
        case 1:
            kill(pid, SIGINT);
            break;
        
        case 2:
            kill(pid, SIGUSR1);
            break;

        default:
            break;
        }
    }

    return 0;
}