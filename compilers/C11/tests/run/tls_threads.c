/* _Thread_local: every thread has its own copy of an object.  Several threads
   count in their own copy; main's copy is not touched. */
#include <stdio.h>
#include <stdlib.h>
#include <_Thread.h>

#define THREADS 4

_Thread_local int counter;                  /* zero in every thread */
_Thread_local long total = 1000;
_Thread_local double scale = 0.5;
_Thread_local char tag;
static _Thread_local int hidden = 7;        /* internal linkage */

struct job {
    int id;
    int steps;
    int counter_before;
    int counter_after;
    long total_after;
    double scale_after;
    int hidden_after;
    int tag_after;
};

static void worker(void *arg)
{
    struct job *j = arg;
    int i;
    j->counter_before = counter;
    for (i = 0; i < j->steps; i++) {
        counter++;
        total += j->id;
        scale *= 2;
        sleep_ms(1);                        /* let the threads interleave */
    }
    counter += 100 * j->id;
    counter -= 1;
    ++counter;
    hidden += counter;
    tag = 'a' + j->id;
    j->counter_after = counter;
    j->total_after = total;
    j->scale_after = scale;
    j->hidden_after = hidden;
    j->tag_after = tag;
}

int main(void)
{
    long tids[THREADS];
    struct job *jobs = malloc(THREADS * sizeof(struct job));
    int i;
    counter = 5;                            /* main's copy */
    total = 2000;
    for (i = 0; i < THREADS; i++) {
        jobs[i].id = i + 1;
        jobs[i].steps = 10 * (i + 1);
        tids[i] = thread_create(worker, &jobs[i]);
    }
    for (i = 0; i < THREADS; i++) thread_join(tids[i]);
    for (i = 0; i < THREADS; i++) {
        struct job *j = &jobs[i];
        printf("thread %d: before=%d after=%d total=%ld scale=%g hidden=%d tag=%c\n",
               j->id, j->counter_before, j->counter_after, j->total_after,
               j->scale_after, j->hidden_after, j->tag_after);
    }
    printf("main: counter=%d total=%ld scale=%g hidden=%d tag=%d\n", counter, total, scale, hidden, tag);
    free(jobs);
    return 0;
}
