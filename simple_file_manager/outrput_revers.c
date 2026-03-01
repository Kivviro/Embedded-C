#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
// linlibs
#include <fcntl.h>
#include <unistd.h>


int readInt(const char *prompt, int *out);
int writeStringToFile();
int readStringFromFile();

void testCursedFunc();

int main()
{
    int choice;

    while(1)
    {
        printf("\nФайловая система:\n");
        printf("1. Создать файл\n");
        printf("2. Чтение файла\n");
        printf("0. Выход\n");
        printf("Выбор: ");

        if (!readInt("", &choice))
            continue;

        if (choice == 0)
            return 0;

        switch (choice)
        {
            case 1:
                writeStringToFile();
                break;

            case 2:
                printf("Чтение файла... В файле записано:\n");
                readStringFromFile();
                break;
        }
    }
}

int writeStringToFile()
{
    printf("Создание файла и запись в него значения\n");
    int fd = open("output.txt", O_CREAT | O_WRONLY, 0644);
    
    if (fd == -1) 
        return 1;

    
    const char *text = "String from file\n";
    size_t len = strlen(text);
    ssize_t written = write(fd, text, len);

    if (written == -1)
    {
        close(fd);
        return 2;
    }
    close(fd);
    return 0;
}

int readStringFromFile()
{
    int fd = open("output.txt", O_RDONLY);
    if (fd == -1)
        return 1;

    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1)
    {
        close(fd);
        return 2;
    }
    
    for (off_t p = file_size - 1; p >= 0; --p)
    {
        char c;
        if (lseek(fd, p, SEEK_SET) == -1)
        {
            close(fd);
            return 3;
        }
        
        if (read(fd, &c, 1) != 1)
        {
            close(fd);
            return 4;
        }

        write(STDOUT_FILENO, &c, 1);
    }
    write(STDOUT_FILENO, "\n", 1);
    close(fd);
    return 0;
}

int readInt(const char *prompt, int *out)
{
    char buf[64];
    char *end;
    long val;

    printf("%s", prompt);

    if (!fgets(buf, sizeof(buf), stdin))
        return 0;

    errno = 0;
    val = strtol(buf, &end, 10);

    if (errno == ERANGE || val < INT_MIN || val > INT_MAX)
        return 0;

    if (end == buf)
        return 0;

    *out = (int)val;
    return 1;
}
