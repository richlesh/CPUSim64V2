/* -lstd: the C library is linked as whole modules (obj/lib/*.asm), so its
   state is shared by all modules: the atexit handler registered in
   multi_lstd_util.c runs when main returns, and rand() continues the
   sequence seeded there. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setup(void);
int next(void);

int main(void) {
    setup();
    int a = next(), b = next();
    srand(5);
    printf("same sequence: %d\n", a == rand() && b == rand());
    char buf[32];
    snprintf(buf, sizeof buf, "%.2f|%5d|%s", 2.5 * 3, 42, "ok");
    printf("%s (%d chars)\n", buf, (int)strlen(buf));
    return 0;
}
