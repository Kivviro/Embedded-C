#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

int main()
{
    const char *path = "/tmp/fifo_rw";

    int fd_rw = open(path, O_RDONLY);
    if (fd_rw == 1)
        return 1;

    
    char buf[100];
    ssize_t n = read(fd_rw, buf, sizeof(buf) - 1);
    if (n == -1)
    {
        close(fd_rw);
        return 1;
    }
    
    buf[n] = '\0';
    printf("Client read: %s\n", buf);

    close(fd_rw);


    if (unlink(path) == -1) 
        return 1;

    return 0;
}