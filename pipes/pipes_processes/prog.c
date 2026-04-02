#include <stdio.h>
#include <unistd.h>

int main() {
    int fd[2];
    pipe(fd);
    pid_t pid = fork();

    if (pid != 0) 
    {
        close(fd[0]);

        write(fd[1], "Hi!", 4);
        close(fd[1]);
    }
    else
    {
        close(fd[1]);

        char buf[100];
        read(fd[0], buf, sizeof(buf));
        printf("Child read: %s\n", buf);

        close(fd[0]);
    } 

    return 0;
}