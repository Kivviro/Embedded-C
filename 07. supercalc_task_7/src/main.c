#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "../lib/calc.h"

//void InputNumbers(int *a, int *b);
int ReadInt(const char *prompt, int *out);

int main(void)
{
    int choice;
    int a;
    int b;
    int res;
    double dres;

    while (1)
    {
        printf("\nМеню функций СуперКалькулятора-3000:\n");
        printf("1. Сложение\n");
        printf("2. Вычитание\n");
        printf("3. Умножение\n");
        printf("4. Деление\n");
        printf("5. Выход\n");
        printf("Выбор: ");

        if (!ReadInt("", &choice))
            continue;

        if (choice == 5)
            return 0;

        if (!ReadInt("Введите первое число: ", &a))
            continue;
        if (!ReadInt("Введите второе число: ", &b))
            continue;

        switch (choice)
        {
            case 1:
                if (add(a, b, &res))
                    printf("%d + %d = %d\n", a, b, res);
                else
                    printf("Переполнение!\n");
                break;
            case 2:
                if (sub(a, b, &res))
                    printf("%d - %d = %d\n", a, b, res);
                else
                    printf("Переполнение!\n");
                break;
            case 3:
                if (mul(a, b, &res))
                    printf("%d * %d = %d\n", a, b, res);
                else
                    printf("Переполнение!\n");
                break;
            case 4:
                if (superdiv(a, b, &dres))
                    printf("%d / %d = %.2f\n", a, b, dres);
                else
                    printf("Ошибка деления!\n");
                break;
        }
    }
}

int ReadInt(const char *prompt, int *out)
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

