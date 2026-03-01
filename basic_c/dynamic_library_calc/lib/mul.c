#include <limits.h>
#include "calc.h"

int mul(int a, int b, int *result)
{
    if (a != 0 && b != 0)
    {
        if (a > INT_MAX / b || a < INT_MIN / b)
            return 0;
    }

    *result = a * b;
    return 1;
}