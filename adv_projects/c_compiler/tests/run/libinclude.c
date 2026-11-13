#include <stdio.h>
#include <foo>
#include <foo.h>
#include <onlyh.h>
#include <nonexistent_header.h>
int main(void) {
    printf("%d %d %d\n", foo_used(4), FOO_SCALE, ONLYH_VALUE);
    return 0;
}
