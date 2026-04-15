#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void signal_override(int sig)
{
    const char *mes = "Signal received\n";

    write(STDOUT_FILENO, mes, strlen(mes));
}

int main()
{
    printf("pid: %d\n", getpid());

    struct sigaction sa;

    sa.sa_sigaction = signal_override;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGUSR1, &sa, NULL);

    while (1)
        pause();

    return 0;
}