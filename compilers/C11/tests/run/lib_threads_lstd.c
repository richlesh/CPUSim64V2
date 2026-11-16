/* <threads.h> linked as library modules (-lstd): the same program as lib_threads.c */
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

struct job { int id; int out; };

static int worker(void *arg)
{
    struct job *j = arg;
    j->out = j->id * 10;
    return j->id + 100;
}

static int exiter(void *arg)
{
    int *p = arg;
    *p = 1;
    thrd_exit(77);
    *p = 2;                     /* not reached */
    return 5;
}

static thrd_t child_seen;
static thrd_t child_created;
static int child_self_equal;

static int whoami(void *arg)
{
    (void)arg;
    child_seen = thrd_current();
    return 0;
}

static int detached_done;

static int detached_fn(void *arg)
{
    struct timespec ts;
    (void)arg;
    ts.tv_sec = 0;
    ts.tv_nsec = 30000000;
    thrd_sleep(&ts, NULL);
    detached_done = 1;
    return 0;
}

static struct job jobs[4];
static int flag;

static once_flag once = ONCE_FLAG_INIT;
static long once_count;

static void once_fn(void)
{
    thrd_yield();
    once_count++;
}

static int once_worker(void *arg)
{
    (void)arg;
    call_once(&once, once_fn);
    if (once_count != 1) printf("call_once not complete\n");
    return 0;
}

static int deep_exit(int n)
{
    if (n == 0) thrd_exit(42);
    return deep_exit(n - 1) + 1;
}

static int deep_worker(void *arg) { (void)arg; return deep_exit(5); }

int main(void)
{
    thrd_t t[4], d, e, c;
    int i, res, r;
    struct timespec ts, rem;

    for (i = 0; i < 4; i++) {
        jobs[i].id = i + 1;
        jobs[i].out = 0;
        r = thrd_create(&t[i], worker, &jobs[i]);
        if (r != thrd_success) printf("create failed %d\n", r);
    }
    for (i = 0; i < 4; i++) {
        res = 0;
        r = thrd_join(t[i], &res);
        printf("join %d: r=%d res=%d out=%d\n", i, r, res, jobs[i].out);
    }
    printf("rejoin: %d\n", thrd_join(t[0], &res) == thrd_error);

    thrd_create(&e, exiter, &flag);
    thrd_join(e, &res);
    printf("thrd_exit: res=%d flag=%d\n", res, flag);
    thrd_create(&e, deep_worker, NULL);
    thrd_join(e, &res);
    printf("deep thrd_exit: res=%d\n", res);

    thrd_create(&c, whoami, NULL);
    child_created = c;
    thrd_join(c, NULL);
    child_self_equal = thrd_equal(child_seen, child_created);
    printf("current in child equals created: %d\n", child_self_equal);
    printf("main != child: %d\n", !thrd_equal(thrd_current(), child_created));
    printf("main equal itself: %d\n", thrd_equal(thrd_current(), thrd_current()));
    printf("join self: %d\n", thrd_join(thrd_current(), NULL) == thrd_error);

    thrd_create(&d, detached_fn, NULL);
    printf("detach: %d\n", thrd_detach(d));
    printf("detach again: %d\n", thrd_detach(d) == thrd_error);
    printf("join detached: %d\n", thrd_join(d, NULL) == thrd_error);
    ts.tv_sec = 0;
    ts.tv_nsec = 200000000;
    rem.tv_sec = 9;
    r = thrd_sleep(&ts, &rem);
    printf("sleep: %d rem=%ld\n", r, (long)rem.tv_sec);
    printf("detached done: %d\n", detached_done);

    {
        struct timespec a, b;
        long us;
        ts.tv_sec = 0;
        ts.tv_nsec = 120000000;
        timespec_get(&a, TIME_UTC);
        thrd_sleep(&ts, NULL);
        timespec_get(&b, TIME_UTC);
        us = (b.tv_sec - a.tv_sec) * 1000000 + (b.tv_nsec - a.tv_nsec) / 1000;
        printf("slept at least 120ms: %d, less than 2s: %d\n", us >= 120000, us < 2000000);
    }

    for (i = 0; i < 4; i++) thrd_create(&t[i], once_worker, NULL);
    for (i = 0; i < 4; i++) thrd_join(t[i], NULL);
    printf("once_count=%ld\n", once_count);

    printf("exit code next\n");
    fflush(stdout);
    thrd_exit(3);               /* main thread: waits for the others, exits with 3 */
    printf("not reached\n");
    return 0;
}
