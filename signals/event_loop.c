#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
    int sig;
    sigset_t set;

    printf("pid: %d\n", getpid());

    
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);

    sigprocmask(SIG_BLOCK, &set, NULL);

    while (1)
    {
        sigwait(&set, &sig);

        if (sig = SIGUSR1)
            printf("SIGUSR1 received\n");
    }

    return 0;
}