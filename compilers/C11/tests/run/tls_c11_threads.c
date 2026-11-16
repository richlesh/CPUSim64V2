/* thread_local (<threads.h>) with thrd_create: every thread has its own
   initialized copy (also through thrd_exit), and main's copy is untouched */
#include <stdio.h>
#include <threads.h>

#define N 4

thread_local int counter = 100;
thread_local int seen[3] = {1, 2, 3};
static int results[N];

static int work(void *arg)
{
    long id = (long)arg;
    int i;
    for (i = 0; i < (int)id + 1; i++) counter++;
    seen[1] += (int)id;
    results[id] = counter * 10 + seen[1];
    if (id == 2) thrd_exit(counter);
    return counter;
}

int main(void)
{
    thrd_t t[N];
    long i;
    int res;
    for (i = 0; i < N; i++) thrd_create(&t[i], work, (void *)i);
    for (i = 0; i < N; i++) {
        thrd_join(t[i], &res);
        printf("thread %ld: res=%d result=%d\n", i, res, results[i]);
    }
    printf("main: counter=%d seen=%d %d %d\n", counter, seen[0], seen[1], seen[2]);
    return 0;
}
