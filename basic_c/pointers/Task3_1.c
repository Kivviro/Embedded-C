#include <stdio.h>

void ChangeThirdByte();
void ChangedCode();
void ArrayViaPointers();
int main(void)
{
    int choice;

    while (1)
    {
        printf("\nMenu:\n");
        printf("1. Change third byte\n");
        printf("2. Changed code\n");
        printf("3. Array via pointers\n");
        printf("4. String and Substring\n");
        printf("0. Exit\n");
        printf("Choose: ");

        scanf("%d", &choice);

        switch (choice)
        {
            case 1:
                ChangeThirdByte();
                break;
            case 2:
                ChangedCode();
                break;
            case 3:
                ArrayViaPointers();
                break;
            case 0:
                return 0;
            default:
                printf("Invalid choice.\n");
        }
    }
    return 0;
}

void ChangeThirdByte() // Ok
{
    int n;
    int x;
    char *ptr = (char*)&n;

    printf("Enter a positive integer num: ");
    scanf("%d", &n);
    printf("Enter the new value for third byte of num: ");
    scanf("%d", &x);

    ptr[2] = x;

    printf("Changed num: ");
    printf("%d\n", n);
    printf("Changed bytes form: ");

    for (int i = 0; i < sizeof(int); i++)
        {
        printf("%u ", ptr[i]);
    }
    printf("\n");
}

void ChangedCode() // Ok
{
    float x = 5.0;
    printf("x = %f, ", x);
    float y = 6.0;
    printf("y = %f\n", y);
    float *xp = &y; // TODO: отредактируйте эту строку, и только правую часть уравнения
    float *yp = &y;
    printf("Результат: %f\n", *xp + *yp);
}

void ArrayViaPointers() // Ok
{
    int arr[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int *ptr = arr;

    for (int i = 0; i < 10; i++)
    {
        printf("%d ", *ptr);
        ptr++;
    }
    printf("\n");
}