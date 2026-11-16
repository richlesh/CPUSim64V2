/*
 * vb_thread.c - threads, processes and locks (included by vb_rt.c), on the
 * C11 compiler's <_Thread.h>.
 *
 * - System.Threading.Thread: a thread runs the delegate of the Thread object
 *   (thr_entry calls each delegate of its invocation list).
 * - While threads run (thr_active > 0) the allocator is locked and the
 *   garbage collector does not run: the collector scans only the main stack
 *   and does not stop the other threads.  It runs again when they have all
 *   ended.
 * - The exception handler chain is thread-local: its head is a word of the
 *   stack (see vb_program in codegen_stmt.c), and every thread has its own
 *   stack, which starts zeroed.
 * - Console output is locked while threads run, so that the lines of
 *   Console.WriteLine are not mixed.
 * - Monitor (SyncLock): a recursive mutex for each locked object, found in a
 *   hash table by the object's address (objects do not move).
 * - CPUSim64.Process.Fork: a child process with a copy of the memory.
 */

/* the thread function: the delegate's methods, with Me (the target) and the argument */
static void thr_entry(void *p) {
    long *r = p, *d = (long *)r[0];
    while (d) {
        if (r[2]) ((void (*)(long, long))d[1])(r[1], d[0]);
        else ((void (*)(long))d[1])(d[0]);
        d = (long *)d[2];
    }
    free(r);
    atomic_decrement(&thr_active);
}

/* starts a thread that calls the delegate (with arg if hasarg); returns its id */
long vbrt_thr_start(long *dlg, long *arg, long hasarg) {
    long *r = malloc(3 * sizeof(long)), tid;
    if (!r) fail("OutOfMemoryException", 0);
    r[0] = (long)dlg;
    r[1] = (long)arg;
    r[2] = hasarg != 0;
    atomic_increment(&thr_active);
    tid = thread_create(thr_entry, r);
    if (tid <= 0) {
        atomic_decrement(&thr_active);
        free(r);
        fail("OutOfMemoryException", "Thread failed to start.");
    }
    return tid;
}

void vbrt_thr_join(long tid) { if (tid > 0) thread_join(tid); }

void vbrt_thr_sleep(long ms) {
    if (ms < -1) fail("ArgumentOutOfRangeException", "Number must be either non-negative and less than or equal to Int32.MaxValue or -1. (Parameter 'millisecondsTimeout')");
    if (ms > 0) sleep_ms(ms);
}

long vbrt_thr_id(void) { return get_pid(); }

long vbrt_env_cores(void) { return get_num_cores(); }

/* Environment.GetEnvironmentVariable: Nothing if it is not set */
long *vbrt_env_get(long *name) {
    char *n, *v;
    long i, len;
    if (!name) fail("ArgumentNullException", "Value cannot be null. (Parameter 'variable')");
    len = name[0];
    n = malloc(len + 1);
    for (i = 0; i < len; i++) n[i] = (char)name[1 + i];
    n[len] = 0;
    v = getenv(n);
    free(n);
    return v ? str_from_c(v) : 0;
}

/* ------------------------------------------------------------ Interlocked */

/* v modulo 2^32 as a signed 32-bit value (every C11 integer type is 64 bits) */
static long wrap32(long v) {
    v &= 0xFFFFFFFF;
    return v >= 0x80000000 ? v - 0x100000000 : v;
}

long vbrt_ilk_add(long *p, long n, long is32) {
    long old, nv;
    do {
        old = *p;
        nv = is32 ? wrap32(old + n) : old + n;
    } while (!atomic_cas(p, old, nv));
    return nv;
}

long vbrt_ilk_inc32(long *p) { return vbrt_ilk_add(p, 1, 1); }
long vbrt_ilk_inc64(long *p) { return vbrt_ilk_add(p, 1, 0); }
long vbrt_ilk_dec32(long *p) { return vbrt_ilk_add(p, -1, 1); }
long vbrt_ilk_dec64(long *p) { return vbrt_ilk_add(p, -1, 0); }

long vbrt_ilk_exchange(long *p, long v) {
    long old;
    do {
        old = *p;
    } while (!atomic_cas(p, old, v));
    return old;
}

long vbrt_ilk_cmpxchg(long *p, long v, long comparand) {
    long old;
    for (;;) {
        old = *p;
        if (old != comparand) return old;
        if (atomic_cas(p, old, v)) return old;
    }
}

long vbrt_ilk_read(long *p) { return *p; }

/* ------------------------------------------------------------ Monitor */

typedef struct MonEntry {
    long obj;
    mutex_t m;
    struct MonEntry *next;
} MonEntry;

#define MON_BUCKETS 256
static MonEntry *mon_table[MON_BUCKETS];
static spinlock_t mon_lock;

static mutex_t *mon_find(long *obj, long create) {
    long h = ((long)obj >> 2) & (MON_BUCKETS - 1);
    MonEntry *e;
    if (!obj) fail("ArgumentNullException", "Value cannot be null. (Parameter 'obj')");
    spinlock_lock(&mon_lock);
    for (e = mon_table[h]; e && e->obj != (long)obj; e = e->next) ;
    if (!e && create) {
        e = malloc(sizeof(MonEntry));
        e->obj = (long)obj;
        mutex_init(&e->m);
        e->next = mon_table[h];
        mon_table[h] = e;
    }
    spinlock_unlock(&mon_lock);
    return e ? &e->m : 0;
}

void vbrt_mon_enter(long *obj) { mutex_lock(mon_find(obj, 1)); }

void vbrt_mon_exit(long *obj) {
    mutex_t *m = mon_find(obj, 0);
    if (!m || m->owner != get_pid())
        fail("InvalidOperationException", "Object synchronization method was called from an unsynchronized block of code.");
    mutex_unlock(m);
}

long vbrt_mon_tryenter(long *obj) { return mutex_trylock(mon_find(obj, 1)) ? -1 : 0; }

/* ------------------------------------------------------------ processes */

long vbrt_proc_fork(void) {
    long pid;
    if (thr_active) fail("InvalidOperationException", "Fork is not allowed while other threads run.");
    fflush(stdout);                 /* the child would get a copy of the output not yet written */
    pid = fork();
    if (pid < 0) fail("InvalidOperationException", "Fork failed.");
    return pid;
}

void vbrt_proc_wait(long pid) { fflush(stdout); wait_pid(pid); }

void vbrt_proc_waitall(void) { fflush(stdout); wait_all(); }

/* ------------------------------------------------------------ binary files */

/* File.ReadAllBytes */
long *vbrt_file_readbytes(long *path) {
    char *p = c_path(path);
    FILE *f = fopen(p, "rb");
    long n = 0, cap = 4096, *a, lens[1], i;
    unsigned char *buf;
    int c;
    if (!f) {
        free(p);
        fail("FileNotFoundException", "Could not find the file.");
    }
    free(p);
    buf = malloc(cap);
    while ((c = fgetc(f)) != EOF) {
        if (n == cap) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
        buf[n++] = (unsigned char)c;
    }
    fclose(f);
    lens[0] = n;
    a = new_array(ARR_TAG(K_BYTE, 1), 1, lens);
    for (i = 0; i < n; i++) a[3 + i] = buf[i];
    free(buf);
    return a;
}

/* File.WriteAllBytes */
void vbrt_file_writebytes(long *path, long *bytes) {
    char *p = c_path(path);
    FILE *f;
    long i;
    if (!bytes) fail("ArgumentNullException", "Value cannot be null. (Parameter 'bytes')");
    f = fopen(p, "wb");
    free(p);
    if (!f) fail("IOException", "The file could not be written.");
    for (i = 0; i < bytes[0]; i++) fputc((int)(bytes[3 + i] & 255), f);
    fclose(f);
}
