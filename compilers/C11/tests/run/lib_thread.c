/* lib/_Thread.h: threads, atomics, spin locks, mutexes, fork and shared memory */
#include <stdio.h>
#include <stdlib.h>
#include <_Thread.h>

#define THREADS 4
#define STEPS 300

static long counter;                /* atomic_increment */
static long plain_total;            /* guarded by the mutex */
static long spin_total;             /* guarded by the spin lock */
static mutex_t mtx;
static spinlock_t spin;

struct job {
    long id;
    long result;
};

static void worker(void *arg)
{
    struct job *j = arg;
    long i;
    for (i = 0; i < STEPS; i++) {
        atomic_increment(&counter);
        mutex_lock(&mtx);
        mutex_lock(&mtx);                       /* recursive */
        plain_total += j->id;
        mutex_unlock(&mtx);
        mutex_unlock(&mtx);
        spinlock_lock(&spin);
        spin_total += 1;
        spinlock_unlock(&spin);
    }
    j->result = j->id * 100;
}

static volatile long woke;

static void sleeper(void *arg)
{
    (void)arg;
    sleep_ms(60000);                            /* ended early by thread_wake */
    woke = 1;
}

int main(void)
{
    long tids[THREADS], i, pid, *shared, tid;
    struct job *jobs = malloc(THREADS * sizeof(struct job));
    long x = 5;
    mutex_init(&mtx);
    spinlock_init(&spin);
    for (i = 0; i < THREADS; i++) {
        jobs[i].id = i + 1;
        jobs[i].result = 0;
        tids[i] = thread_create(worker, &jobs[i]);
        if (tids[i] < 0) printf("thread_create failed\n");
    }
    for (i = 0; i < THREADS; i++) thread_join(tids[i]);
    printf("counter=%ld mutex=%ld spin=%ld\n", counter, plain_total, spin_total);
    for (i = 0; i < THREADS; i++) printf("result %ld = %ld\n", i, jobs[i].result);
    {
        /* one call per statement: the order of evaluation of arguments is unspecified */
        int c1 = atomic_cas(&x, 5, 9), c2 = atomic_cas(&x, 5, 1);
        long a, d;
        int t1, t2;
        printf("cas %d %d x=%ld\n", c1, c2, x);
        a = atomic_add(&x, 10);
        d = atomic_decrement(&x);
        printf("add %ld dec %ld\n", a, d);
        t1 = spinlock_trylock(&spin);
        t2 = spinlock_trylock(&spin);
        printf("trylock %d %d\n", t1, t2);
    }
    spinlock_unlock(&spin);

    tid = thread_create(sleeper, 0);
    sleep_ms(50);
    thread_wake(tid);
    thread_join(tid);
    printf("woke=%ld\n", woke);

    /* processes: the child's copy of the memory is its own, the shared memory is not */
    shared = alloc_shared(3);
    shared[0] = shared[1] = shared[2] = 0;
    counter = 1;
    for (i = 0; i < 3; i++) {
        pid = fork();
        if (pid == 0) {
            counter = 1000;
            atomic_add(&shared[0], i + 1);
            exit(0);
        }
        if (pid < 0) printf("fork failed\n");
    }
    wait_all();
    printf("shared=%ld counter=%ld\n", shared[0], counter);
    pid = fork();
    if (pid == 0) {
        shared[1] = get_pid();
        exit(3);
    }
    wait_pid(pid);
    printf("child pid ok: %d\n", shared[1] == pid);
    printf("cores ok: %d\n", get_num_cores() >= 1);
    free(jobs);
    return 0;
}
