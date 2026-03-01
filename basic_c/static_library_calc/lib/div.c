#include <limits.h>
#include <stdio.h>

#include "calc.h"

int superdiv(int a, int b, double *result)
{
    if (b == 0)
        return 0;

    if (a == INT_MIN && b == -1)
        return 0;

    *result = (double)a / b;
    return 1;
}