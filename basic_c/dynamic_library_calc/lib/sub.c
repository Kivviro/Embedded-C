#include <limits.h>
#include "calc.h"

int sub(int a, int b, int *result)
{
    if ((b < 0 && a > INT_MAX + b) || (b > 0 && a < INT_MIN + b))
        return 0;

    *result = a - b;
    return 1;
}