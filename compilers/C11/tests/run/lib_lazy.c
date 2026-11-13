#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <setjmp.h>
/* Including every library but using only strlen must not compile the rest. */
int main(void) {
    printf("%d\n", (int)strlen("lazy"));
    return 0;
}
