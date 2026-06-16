#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int main()
{
    const char *path = "/tmp/fifo_rw";

    mkfifo(path, 0666);
    
    int fd_rw = open(path, O_WRONLY);
    if (fd_rw == -1)
        return 1;

    char bug[100];
    ssize_t n = write(fd_rw, "Hi!", 4);
    if(n == -1)
    {
        close(fd_rw);
        return 1;
    }

    close(fd_rw);
    return 0;
}