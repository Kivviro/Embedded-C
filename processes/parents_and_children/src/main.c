#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void print_processes_info(const char *name)
{
    printf("%s: pid = %d, ppid = %d\n", name, getpid(), getppid());
}

int main()
{
    pid_t pid_1;
    pid_t pid_2;

    pid_1 = fork();

    if (pid_1 < 0)
        _exit(1);

    if (pid_1 == 0)
    {
        print_processes_info("Process 1");

        pid_t pid_3 = fork();

        if (pid_3 == 0)
        {
            print_processes_info("Process 3");
            _exit(0);
        }
        
        pid_t pid_4 = fork();

        if (pid_4 == 0)
        {
            print_processes_info("Proccess 4");
            _exit(0);
        }

        wait(NULL);
        wait(NULL);
        
        _exit(0);
    }

    pid_2 = fork();

    if (pid_2 < 0)
        _exit(0);
    
    if (pid_2 == 0)
    {
        print_processes_info("Process 2");

        pid_t pid_5 = fork();

        if (pid_5 == 0)
        {
            print_processes_info("Process 5");
            _exit(0);
        }
        
        wait(NULL);

        _exit(0);
    }

    wait(NULL);
    wait(NULL);
    
    return 0;
}