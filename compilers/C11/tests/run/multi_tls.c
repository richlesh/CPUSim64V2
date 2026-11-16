/* _Thread_local across modules: objects defined in two modules, an extern
   declaration of an object of another module, internal-linkage objects of the
   same name in different modules, threads created by a third module. */
#include <stdio.h>
#include <stdlib.h>
#include "../multi/multi_tls.h"

_Thread_local int tls_own = 77;                 /* used by multi_tls_a.c */
static _Thread_local int hidden = -1;           /* a different object than the one in multi_tls_a.c */

struct job {
    int id;
    int counter, table2, own, other, hidden_bump, hidden;
    int label_ok;
};

static void worker(void *arg)
{
    struct job *j = arg;
    tls_bump(j->id);
    tls_bump(j->id);
    tls_table[2] += j->id;
    tls_own += j->id;
    hidden -= j->id;
    j->counter = tls_counter;
    j->table2 = tls_table[2];
    j->own = tls_own;
    j->other = tls_other();
    tls_hidden_bump();
    j->hidden_bump = tls_hidden_bump();
    j->hidden = hidden;
    j->label_ok = tls_label[7] == 'a';
}

int main(void)
{
    struct job *jobs = calloc(3, sizeof(struct job));
    int i;
    printf("main: counter=%d table=%d,%d,%d,%d label=%s own=%d hidden=%d\n", tls_counter, tls_table[0],
           tls_table[1], tls_table[2], tls_table[3], tls_label, tls_own, hidden);
    for (i = 0; i < 3; i++) jobs[i].id = i + 1;
    tls_run(worker, jobs, 3, sizeof(struct job));
    for (i = 0; i < 3; i++) {
        struct job *j = &jobs[i];
        printf("thread %d: counter=%d table[2]=%d own=%d other=%d hidden_bump=%d hidden=%d label-ok=%d\n", j->id,
               j->counter, j->table2, j->own, j->other, j->hidden_bump, j->hidden, j->label_ok);
    }
    tls_bump(1);
    printf("main: counter=%d table[2]=%d own=%d hidden=%d hb=%d\n", tls_counter, tls_table[2], tls_own, hidden,
           tls_hidden_bump());
    free(jobs);
    return 0;
}
