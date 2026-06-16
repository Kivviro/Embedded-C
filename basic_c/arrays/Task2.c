#include <stdio.h>

void SquareMatrixOfN();
void ReverseArrayOutput();
void TriangleMatrix();
void SnailMatrix();
int MyAbs();

#define N 5

int main()
{
    int choice;

    while (1)
    {
        printf("\nMenu:\n");
        printf("1. Square matrix of N\n");
        printf("2. Reverse array output of N\n");
        printf("3. Triangle matrix of N\n");
        printf("4. Snail matrix of N\n");
        printf("0. Exit\n");
        printf("Choose: ");

        scanf("%d", &choice);

        switch (choice)
        {
            case 1:
                SquareMatrixOfN();
                break;
            case 2:
                ReverseArrayOutput();
                break;
            case 3:
                TriangleMatrix();
                break;
            case 4:
                SnailMatrix();
                break;
            case 0:
                return 0;
            default:
                printf("Invalid choice.\n");
        }
    }
    return 0;
}

void SquareMatrixOfN() // Ok
{
    int matrix[N][N];
    int val = 1;
    printf("Simple Matrxi:\n");
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            matrix[i][j] = val++;
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }
}

void ReverseArrayOutput() // Ok
{
    int array[N];

    for (int i = 0; i < N; i++)
        array[i] = i + 1;

    printf("Reverse array: ");
    for (int i = 0; i < N; i++)
    {
        printf("%d ",array[N-i-1]);
    }
    printf("\n");
}

void TriangleMatrix() // Ok
{
    int matrix[N][N];

    printf("Triangle Matrix:\n");
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            if (i + j >= N - 1)
                matrix[i][j] = 1;
            else
                matrix[i][j] = 0;
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }
}

void SnailMatrix() // Ok
{
    int matrix[N][N];

    printf("Snail Matrix filling zeroes:\n");
    for (int i = 0; i < N; i++) // loop of zero filling
        for (int j = 0; j < N; j++)
            matrix[i][j] = 0;

    for (int ik = 0; ik < N; ik++) // Main block
    {
        for (int jk = 0; jk < N; jk++)
        {
            int i = ik + 1;
            int j = jk + 1;

            int switcher =  (j - i + N) / N;
            int Ic = MyAbs(i - N / 2  - 1) + (i - 1)/(N / 2) * ((N - 1) % 2);
            int Jc = MyAbs(j - N / 2  - 1) + (j - 1)/(N / 2) * ((N - 1) % 2);
            int ring = N / 2 - (MyAbs(Ic - Jc) + Ic + Jc) / 2;
            int Xs = i - ring + j - ring - 1;
            int coef =  4 * ring * (N - ring);
            matrix[ik][jk] =  coef + switcher * Xs + MyAbs(switcher - 1) * (4 * (N - 2 * ring) - 2 - Xs);
        }
    }

    for (int ik = 0; ik < N; ik++) // Output
    {
        for (int jk = 0; jk < N; jk++)
        {
            printf( "%02d ", matrix[ik][jk]);
        }
        printf("\n");
    }
}

int MyAbs(int x) // module's own function (well, why not?)) )
{
    return x < 0 ? -x : x;
}