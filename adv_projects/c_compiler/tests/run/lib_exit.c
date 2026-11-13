#include <stdio.h>
#include <stdlib.h>

/* exit() called from a nested function runs the atexit handlers */
static void handler(void) { printf("handler ran\n"); }

static void fail(int code) {
    printf("fail(%d)\n", code);
    exit(code);
}

int main(void) {
    atexit(handler);
    fail(3);
    printf("not reached\n");
    return 0;
}
