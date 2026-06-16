#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
    int status;
    pid_t pid = fork();

    if (pid < 0)
        _exit(1);

    if (pid == 0)
    {
        printf("Child:\n");
        printf("pid = %d\n", getpid());
        printf("ppid = %d\n", getppid());

        _exit(5);
    }
    else
    {
        printf("Parent:\n");
        printf("pid = %d\n", getpid());
        printf("ppid = %d\n", getppid());
        wait(&status);
        printf("status = %d", WEXITSTATUS(status));
    }
    
    return 0;
}