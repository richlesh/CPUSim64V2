#include <stdio.h>
int foo_used(int x) { return -x; }
#include <foo.h>
int main(void) {
    printf("%d\n", foo_used(5));
    return 0;
}
