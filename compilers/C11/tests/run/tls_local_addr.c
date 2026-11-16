/* block-scope static _Thread_local, extern declarations at both scopes,
   tentative definitions, and the address of a thread-local object */
#include <stdio.h>
#include <stdlib.h>
#include <_Thread.h>

extern _Thread_local int ext;                   /* declared first, defined below */
_Thread_local int ext;                          /* tentative definition */
_Thread_local int ext;                          /* and again */
_Thread_local int ext2 = 20;
_Thread_local int grid[3][3];

static int next_id(void)
{
    static _Thread_local int id = 100;          /* one per thread */
    return ++id;
}

static int counted(void)
{
    static _Thread_local int calls;             /* zero */
    extern _Thread_local int ext2;              /* block-scope extern */
    calls++;
    ext2 += calls;
    return calls;
}

struct job {
    int id;
    int *addr;                                  /* &ext as seen by the thread */
    int *addr_grid;
    int first, second, calls, ext2;
    int seen_by_pointer;
    int *main_ptr;                              /* main's &ext */
    int via_main_ptr;
};

static void worker(void *arg)
{
    struct job *j = arg;
    int *p = &ext;
    int (*row)[3] = grid;
    ext = j->id * 10;
    j->addr = &ext;
    j->addr_grid = &grid[1][1];
    *p += 1;                                    /* through the pointer */
    row[1][1] = j->id;
    grid[2][0] = row[1][1] * 2;
    j->seen_by_pointer = *p == ext && *j->addr == ext;
    j->first = next_id();
    j->second = next_id();
    counted();
    counted();
    j->calls = counted();
    j->ext2 = ext2;
    j->first += grid[2][0] + grid[1][1];
    /* a thread-local address is the same number in every thread; it names the
       copy of the thread that uses it, so main's pointer reaches the thread's own object */
    j->via_main_ptr = *j->main_ptr;
}

int main(void)
{
    long t[3];
    struct job *jobs = malloc(3 * sizeof(struct job));      /* not on main's stack */
    int i, *mine = &ext, same = 1;
    ext = 1;
    printf("main: ext=%d ext2=%d id=%d\n", ext, ext2, next_id());
    for (i = 0; i < 3; i++) {
        jobs[i].id = i + 1;
        jobs[i].main_ptr = mine;
        t[i] = thread_create(worker, &jobs[i]);
    }
    for (i = 0; i < 3; i++) thread_join(t[i]);
    for (i = 0; i < 3; i++) {
        printf("thread %d: first=%d second=%d calls=%d ext2=%d ptr-ok=%d via-main-ptr=%d\n", jobs[i].id, jobs[i].first,
               jobs[i].second, jobs[i].calls, jobs[i].ext2, jobs[i].seen_by_pointer, jobs[i].via_main_ptr);
        if (jobs[i].addr != mine || jobs[i].addr_grid != &grid[1][1]) same = 0;
    }
    printf("same address in every thread: %d\n", same);
    printf("main: ext=%d ext2=%d id=%d grid=%d,%d\n", ext, ext2, next_id(), grid[1][1], grid[2][0]);
    free(jobs);
    printf("main: &ext==mine %d sizeof(grid)=%d\n", &ext == mine, (int)sizeof(grid));
    return 0;
}
