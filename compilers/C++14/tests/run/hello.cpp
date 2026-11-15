// A C-compatible program: the first translator passes it through to the C11 compiler.
#include <stdio.h>

int main(void)
{
    int i;
    printf("Hello from C++\n");
    for (i = 1; i <= 3; i++)
        printf("%d squared is %d\n", i, i * i);
    return 0;
}
