#include <stdio.h>
#include <setjmp.h>

static jmp_buf env, retry_env;
static volatile int depth_reached;

static void deep(int n) {
    if (n == 0) longjmp(env, 42);
    depth_reached = n;
    deep(n - 1);
    printf("not reached\n");
}

static void jump_zero(void) { longjmp(env, 0); }

static int attempts;
static void flaky(void) {
    attempts++;
    if (attempts < 3) longjmp(retry_env, attempts);
}

static int nested_inner(jmp_buf b) {
    longjmp(b, 7);
    return 0;
}

int main(void) {
    volatile int local = 1;
    int r = setjmp(env);
    if (r == 0) {
        local = 2;
        printf("first return %d\n", r);
        deep(100);
    } else {
        printf("longjmp returned %d local=%d depth=%d\n", r, local, depth_reached);
    }

    /* longjmp with value 0 makes setjmp return 1 */
    if (setjmp(env) == 0) jump_zero();
    else printf("zero became 1\n");

    /* retry loop */
    int code = setjmp(retry_env);
    printf("attempt after code %d\n", code);
    flaky();
    printf("attempts=%d\n", attempts);

    /* an independent buffer passed to another function; setjmp as the
       controlling expression of a switch */
    jmp_buf local_buf;
    switch (setjmp(local_buf)) {
    case 0:
        nested_inner(local_buf);
        printf("not reached\n");
        break;
    case 7:
        printf("switch got 7\n");
        break;
    default:
        printf("unexpected\n");
    }
    return 0;
}
