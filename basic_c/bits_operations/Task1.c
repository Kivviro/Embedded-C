#include <stdio.h>

void printBinaryPositive();
void printBinaryNegative();
void countOnes();
void replaceThirdByte();

int main()
{
    int choice;

    while (1)
    {
        printf("\nMenu:\n");
        printf("1. Print binary of positive integer\n");
        printf("2. Print binary of negative integer\n");
        printf("3. Count ones in positive integer\n");
        printf("4. Replace third byte in positive integer\n");
        printf("0. Exit\n");
        printf("Choose: ");

        scanf("%d", &choice);

        switch (choice)
        {
            case 1:
                printBinaryPositive();
                break;
            case 2:
                printBinaryNegative();
                break;
            case 3:
                countOnes();
                break;
            case 4:
                replaceThirdByte();
                break;
            case 0:
                return 0;
            default:
                printf("Invalid choice.\n");
        }
    }
    return 0;
}

void printBinary(int n)
{
    unsigned mask = 1U << (sizeof(n) * 8 - 1);
    int started = 0;

    for (; mask; mask >>= 1)
    {
        int bit = (n & mask);

        if (bit || started)
        {
            putchar(bit ? '1' : '0');
            started = 1;
        }
    }

    if (!started)
        putchar('0');
}

void printBinaryPositive()
{
    int n;
    printf("Enter a positive integer: ");
    scanf("%d", &n);

    if (n < 0)
    {
        printf("Number is not positive.\n");
        return;
    }

    printf("Binary: ");
    printBinary(n);
    printf("\n");
}

void printBinaryNegative()
{
    int n;
    printf("Enter a negative integer: ");
    scanf("%d", &n);

    if (n >= 0)
    {
        printf("Number is not negative.\n");
        return;
    }

    printf("Binary: ");
    printBinary(n);
    printf("\n");
}

void countOnes()
{
    int n;
    printf("Enter a positive integer: ");
    scanf("%d", &n);

    if (n < 0)
    {
        printf("Number is not positive.\n");
        return;
    }

    int count = 0;
    unsigned mask = 1U << (sizeof(n) * 8 - 1);

    for (; mask; mask >>= 1)
    {
        if (n & mask)
            count++;
    }

    printf("Binary: ");
    printBinary(n);
    printf("\n");
    printf("Count of ones: %d\n", count);
}

void replaceThirdByte()
{
    int n;
    unsigned char newByte;

    printf("Enter positive integer: ");
    scanf("%d", &n);

    if (n < 0)
    {
        printf("Number is not positive.\n");
        return;
    }

    printf("Enter value for third byte (0–255): ");
    scanf("%hhu", &newByte);

    unsigned int mask = 0xFF << (8 * 2);
    n = (n & ~mask) | ((unsigned int)newByte << (8 * 2));

    printf("Result number: %d\n", n);
    printf("Binary: ");
    printBinary(n);
    printf("\n");
}