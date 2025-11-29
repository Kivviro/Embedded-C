#include <stdio.h>

#define MAX_STR 100
#define MAX_SUB 10

char* FindSubstring(char str[], char substr[])
{
    int j;
    for (int i = 0; str[i] != '\0'; i++)
    {
        for (j = 0; substr[j] != '\0'; j++)
        {
            if (str[i + j] != substr[j])
                break;
        }
        if (substr[j] == '\0')
            return &str[i];
    }
    return NULL;
}

int main(void)
{
    char string[MAX_STR];
    char substring[MAX_SUB];
    char *ptr;

    printf("Input string: ");
    fgets(string, sizeof(string), stdin);

    for (int i = 0; string[i] != '\0'; i++)
        if (string[i] == '\n') { string[i] = '\0'; break; }

    printf("Input substring: ");
    fgets(substring, sizeof(substring), stdin);

    for (int i = 0; substring[i] != '\0'; i++)
        if (substring[i] == '\n'){ substring[i] = '\0'; break; }

    ptr = FindSubstring(string, substring);

    if (ptr != NULL)
        printf("Substring is real! Pointer is " "returned to start of the substring: %s\n", ptr);
    else
        printf("Substring is lie :(.\n");

    return 0;
}











// I hate working with strings :'(