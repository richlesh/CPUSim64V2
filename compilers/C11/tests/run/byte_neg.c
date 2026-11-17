/* a negative byte address is a run time error */
#include <stdio.h>

int main(void) {
    printf("before\n");
    long v = __ldb(-8);
    printf("not reached %ld\n", v);
    return 0;
}
