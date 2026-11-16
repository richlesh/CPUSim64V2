/* <threads.h>: mutexes, condition variables, thread-specific storage */
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#define WORKERS 4
#define STEPS 200

static mtx_t plain, rec, timed;
static long total_plain, total_rec;   /* one counter per mutex */

static int counter_worker(void *arg)
{
    long i;
    (void)arg;
    for (i = 0; i < STEPS; i++) {
        mtx_lock(&plain);
        total_plain++;
        mtx_unlock(&plain);
        mtx_lock(&rec);
        mtx_lock(&rec);
        total_rec++;
        mtx_unlock(&rec);
        mtx_unlock(&rec);
    }
    return 0;
}

/* the main thread holds `timed`, this thread tries it */
static int try_result, timedlock_result, unlock_other;
static int other_trylock(void *arg)
{
    struct timespec ts;
    (void)arg;
    try_result = mtx_trylock(&timed);
    timespec_get(&ts, TIME_UTC);
    ts.tv_nsec += 50000000;
    if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; }
    timedlock_result = mtx_timedlock(&timed, &ts);
    unlock_other = mtx_unlock(&timed);          /* not the owner: error */
    return 0;
}

/* ---- producer / consumer with a queue of 1 slot */
static mtx_t qm;
static cnd_t not_empty, not_full;
static int slot, has_item, consumed_sum, consumed_count;

static int producer(void *arg)
{
    int i;
    (void)arg;
    for (i = 1; i <= 20; i++) {
        mtx_lock(&qm);
        while (has_item) cnd_wait(&not_full, &qm);
        slot = i;
        has_item = 1;
        cnd_signal(&not_empty);
        mtx_unlock(&qm);
    }
    return 0;
}

static int consumer(void *arg)
{
    int n;
    (void)arg;
    for (n = 0; n < 20; n++) {
        mtx_lock(&qm);
        while (!has_item) cnd_wait(&not_empty, &qm);
        consumed_sum += slot;
        consumed_count++;
        has_item = 0;
        cnd_signal(&not_full);
        mtx_unlock(&qm);
    }
    return 0;
}

/* ---- broadcast */
static mtx_t gm;
static cnd_t gate;
static int gate_open, passed, waiting;

static int gate_waiter(void *arg)
{
    (void)arg;
    mtx_lock(&gm);
    waiting++;
    while (!gate_open) cnd_wait(&gate, &gm);
    passed++;
    mtx_unlock(&gm);
    return 0;
}

/* ---- tss */
static tss_t key, key2;
static long dtor_calls, dtor_sum;
static mtx_t dm;

static void dtor(void *p)
{
    long *v = p;
    mtx_lock(&dm);
    dtor_calls++;
    dtor_sum += *v;
    mtx_unlock(&dm);
    free(p);
}

static long redtor_calls;
static void redtor(void *p)
{
    /* sets a new value each time: called up to TSS_DTOR_ITERATIONS times */
    redtor_calls++;
    tss_set(key2, p);
}

static int tss_worker(void *arg)
{
    long id = (long)arg;
    long *v = malloc(sizeof(long));
    *v = id * 100;
    if (tss_get(key) != NULL) printf("tss not empty\n");
    tss_set(key, v);
    thrd_yield();
    if (*(long *)tss_get(key) != id * 100) printf("tss value wrong\n");
    if (id == 3) thrd_exit(9);
    return 0;
}

static int redtor_worker(void *arg)
{
    tss_set(key2, arg);
    return 0;
}

int main(void)
{
    thrd_t t[WORKERS], a, b;
    long i;
    int r, res;
    struct timespec ts, t0, t1;
    static int dummy;

    printf("init plain=%d rec=%d timed=%d bad=%d\n",
           mtx_init(&plain, mtx_plain), mtx_init(&rec, mtx_recursive),
           mtx_init(&timed, mtx_timed), mtx_init(&dm, 99));
    mtx_init(&dm, mtx_plain);
    for (i = 0; i < WORKERS; i++) thrd_create(&t[i], counter_worker, NULL);
    for (i = 0; i < WORKERS; i++) thrd_join(t[i], NULL);
    printf("total plain=%ld rec=%ld\n", total_plain, total_rec);

    mtx_lock(&timed);
    thrd_create(&a, other_trylock, NULL);
    thrd_join(a, NULL);
    printf("trylock busy=%d timedlock timedout=%d unlock by other=%d\n",
           try_result == thrd_busy, timedlock_result == thrd_timedout, unlock_other == thrd_error);
    printf("owner trylock on non-recursive: %d\n", mtx_trylock(&timed) == thrd_busy);
    mtx_unlock(&timed);
    printf("trylock free: %d\n", mtx_trylock(&timed));
    mtx_unlock(&timed);
    printf("recursive trylock: %d %d\n", mtx_trylock(&rec), mtx_trylock(&rec));
    mtx_unlock(&rec);
    mtx_unlock(&rec);

    timespec_get(&ts, TIME_UTC);
    ts.tv_sec += 1;
    printf("timedlock free: %d\n", mtx_timedlock(&timed, &ts));
    mtx_unlock(&timed);

    mtx_init(&qm, mtx_plain);
    cnd_init(&not_empty);
    cnd_init(&not_full);
    thrd_create(&a, consumer, NULL);
    thrd_create(&b, producer, NULL);
    thrd_join(a, NULL);
    thrd_join(b, NULL);
    printf("consumed %d items, sum %d\n", consumed_count, consumed_sum);

    mtx_init(&gm, mtx_plain);
    cnd_init(&gate);
    for (i = 0; i < WORKERS; i++) thrd_create(&t[i], gate_waiter, NULL);
    for (;;) {
        mtx_lock(&gm);
        r = waiting;
        mtx_unlock(&gm);
        if (r == WORKERS) break;
        thrd_yield();
    }
    thrd_sleep(&(struct timespec){0, 30000000}, NULL);
    mtx_lock(&gm);
    printf("passed before broadcast: %d\n", passed);
    gate_open = 1;
    cnd_broadcast(&gate);
    mtx_unlock(&gm);
    for (i = 0; i < WORKERS; i++) thrd_join(t[i], NULL);
    printf("passed after broadcast: %d\n", passed);

    /* cnd_timedwait: nobody signals */
    mtx_lock(&qm);
    timespec_get(&t0, TIME_UTC);
    ts = t0;
    ts.tv_nsec += 100000000;
    if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; }
    r = cnd_timedwait(&not_empty, &qm, &ts);
    timespec_get(&t1, TIME_UTC);
    printf("timedwait: %d, waited >= 100ms: %d, mutex held again: %d\n", r == thrd_timedout,
           (t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_nsec - t0.tv_nsec) / 1000000 >= 100,
           mtx_trylock(&qm) == thrd_busy);
    mtx_unlock(&qm);
    cnd_destroy(&gate);
    mtx_destroy(&gm);

    /* thread-specific storage */
    printf("tss_create: %d\n", tss_create(&key, dtor));
    tss_create(&key2, redtor);
    printf("main get: %d\n", tss_get(key) == NULL);
    for (i = 1; i <= 3; i++) thrd_create(&t[i], tss_worker, (void *)i);
    for (i = 1; i <= 3; i++) {
        res = -1;
        thrd_join(t[i], &res);
        printf("tss worker %ld res=%d\n", i, res);
    }
    printf("destructors: %ld calls, sum %ld\n", dtor_calls, dtor_sum);
    thrd_create(&a, redtor_worker, &dummy);
    thrd_join(a, NULL);
    printf("redtor calls: %ld (limit %d)\n", redtor_calls, TSS_DTOR_ITERATIONS);
    tss_delete(key);
    printf("set after delete: %d\n", tss_set(key, &dummy) == thrd_error);
    printf("main set: %d\n", tss_set(key2, &dummy));
    printf("main get back: %d\n", tss_get(key2) == &dummy);
    return 0;
}
