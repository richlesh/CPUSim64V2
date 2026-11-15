/*
 * vb_gc.c - the garbage collector (included by vb_rt.c): mark and sweep,
 * not moving.
 *
 * The heap is a set of chunks of CHUNK_PAGES pages of PAGE words, taken
 * from the simulator's heap.  Each page holds cells of one size class
 * (object words + the 2 header words); larger objects have blocks of their
 * own.  A cell is free if its tag word is 0 (the GC word then links the free
 * list).  The GC word of an object (p[-1]) holds the mark bit.
 *
 * Marking is precise inside the heap (the reference fields of a class or
 * structure are listed in its descriptor, array tags tell whether the
 * elements are references) and conservative for the roots: every word of
 * the stack (generated code and the library's C frames) and of the
 * program's #global data that points into an object (anywhere inside it)
 * keeps it alive.
 *
 * While other threads run (vb_thread.c) allocation is locked and the
 * collector does not run.
 */

#define PAGE 4096
#define CHUNK_PAGES 16
#define CHUNKW (PAGE * CHUNK_PAGES)
#define NCLASS 13
#define MIN_THRESHOLD 1048576

static const long class_words[NCLASS] = { 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256 };

typedef struct {
    long *base;
    long pc[CHUNK_PAGES];       /* the size class of each page, -1 unused */
    long used;                  /* pages handed out so far */
} Chunk;

typedef struct {
    long *base;
    long words;
} Large;

static long *free_lists[NCLASS];
static Chunk **chunks;
static long nchunks, cap_chunks;
static Large *larges;
static long nlarges, cap_larges;
static long *free_pages;        /* pages of no class: chunk index * CHUNK_PAGES + page */
static long nfree_pages, cap_free_pages;
static long gc_allocated, gc_threshold = MIN_THRESHOLD, gc_live;
static long gc_stress;
static long *gc_gstart, *gc_gend, *gc_stack_top;
static long gc_running, gc_collections;
static long **mark_stack;
static long nmark, cap_mark;

void vbrt_gc_init(long *gstart, long *gend, long *stack_top) {
    gc_gstart = gstart;
    gc_gend = gend;
    gc_stack_top = stack_top;
}

void vbrt_gc_stress(long on) { gc_stress = on; }

static void out_of_memory(void) {
    if (gc_running) {
        fputs("vbrt: out of memory during garbage collection\n", stderr);
        exit(1);
    }
    fail("OutOfMemoryException", 0);
}

/* ------------------------------------------------------------ pages */

static long *new_chunk(void) {
    long *b = calloc(CHUNKW, sizeof(long));
    Chunk *c;
    long i, at;
    if (!b) return 0;
    c = calloc(1, sizeof(Chunk));
    if (!c) {
        free(b);
        return 0;
    }
    c->base = b;
    for (i = 0; i < CHUNK_PAGES; i++) c->pc[i] = -1;
    if (nchunks == cap_chunks) {
        cap_chunks = cap_chunks ? 2 * cap_chunks : 16;
        chunks = realloc(chunks, sizeof(Chunk *) * cap_chunks);
    }
    /* sorted by address */
    for (at = nchunks; at > 0 && (long)chunks[at - 1]->base > (long)b; at--) chunks[at] = chunks[at - 1];
    chunks[at] = c;
    nchunks++;
    /* the indices of the free pages change: they are (chunk, page) pairs */
    for (i = 0; i < nfree_pages; i++)
        if (free_pages[i] / CHUNK_PAGES >= at) free_pages[i] += CHUNK_PAGES;
    return b;
}

static void push_free_page(long id) {
    if (nfree_pages == cap_free_pages) {
        cap_free_pages = cap_free_pages ? 2 * cap_free_pages : 64;
        free_pages = realloc(free_pages, sizeof(long) * cap_free_pages);
    }
    free_pages[nfree_pages++] = id;
}

/* a page for the size class c, its cells on the free list */
static int alloc_page(long c) {
    long id = -1, i, cw = class_words[c], n;
    long *p;
    Chunk *ch;
    if (nfree_pages) id = free_pages[--nfree_pages];
    else {
        for (i = 0; i < nchunks && id < 0; i++)
            if (chunks[i]->used < CHUNK_PAGES) id = i * CHUNK_PAGES + chunks[i]->used++;
        if (id < 0) {
            long *b = new_chunk();
            if (!b) return 0;
            for (i = 0; i < nchunks; i++)
                if (chunks[i]->base == b) id = i * CHUNK_PAGES + chunks[i]->used++;
        }
    }
    ch = chunks[id / CHUNK_PAGES];
    ch->pc[id % CHUNK_PAGES] = c;
    p = ch->base + (id % CHUNK_PAGES) * PAGE;
    n = PAGE / cw;
    for (i = n - 1; i >= 0; i--) {
        long *cell = p + i * cw;
        cell[0] = 0;
        cell[1] = (long)free_lists[c];
        free_lists[c] = cell;
    }
    return 1;
}

/* ------------------------------------------------------------ finding objects */

/* the object that contains the address w (or that starts there), or 0 */
static long *find_object(long w) {
    long lo = 0, hi = nchunks - 1;
    while (lo <= hi) {
        long mid = (lo + hi) / 2;
        long b = (long)chunks[mid]->base;
        if (w < b) hi = mid - 1;
        else if (w >= b + CHUNKW) lo = mid + 1;
        else {
            long page = (w - b) / PAGE, c = chunks[mid]->pc[page], cw, k;
            long *pb, *cell;
            if (c < 0) return 0;
            cw = class_words[c];
            pb = chunks[mid]->base + page * PAGE;
            k = (w - (long)pb) / cw;
            if ((k + 1) * cw > PAGE) return 0;
            cell = pb + k * cw;
            if (cell[0] == 0) return 0;
            return cell + 2;
        }
    }
    lo = 0;
    hi = nlarges - 1;
    while (lo <= hi) {
        long mid = (lo + hi) / 2;
        long b = (long)larges[mid].base;
        if (w < b) hi = mid - 1;
        else if (w >= b + larges[mid].words) lo = mid + 1;
        else return larges[mid].base[0] ? larges[mid].base + 2 : 0;
    }
    return 0;
}

/* ------------------------------------------------------------ marking */

static void mark_value(long w) {
    long *o;
    if (w == 0) return;
    o = find_object(w);
    if (!o || (o[-1] & 1)) return;
    o[-1] |= 1;
    if (nmark == cap_mark) {
        cap_mark = cap_mark ? 2 * cap_mark : 1024;
        mark_stack = realloc(mark_stack, sizeof(long *) * cap_mark);
        if (!mark_stack) out_of_memory();
    }
    mark_stack[nmark++] = o;
}

/* array elements that are references: String, Object, arrays, classes,
   structures (boxes), interfaces */
static int ref_kind(long k) { return k == K_STRING || k == K_OBJECT || k == K_ARRAY || k >= 20; }

static void mark_children(long *o) {
    long t = o[-2], i;
    if (t > 255) {
        long *d = (long *)t, *slots;
        if (d[0] == D_ENUM) return;
        slots = (long *)d[10];
        for (i = 0; i < d[9]; i++) mark_value(o[slots[i]]);
    } else if (t < 0) {
        long ek = ((-t) >> 8) & 255, *e = o + 2 + o[1];
        if (!ref_kind(ek)) return;
        for (i = 0; i < o[0]; i++) mark_value(e[i]);
    }
}

/* ------------------------------------------------------------ sweeping */

static void sweep(void) {
    long i, k, c;
    gc_live = 0;
    for (c = 0; c < NCLASS; c++) free_lists[c] = 0;
    for (i = 0; i < nchunks; i++) {
        Chunk *ch = chunks[i];
        long pg;
        for (pg = 0; pg < ch->used; pg++) {
            long cw, n, live = 0;
            long *pb = ch->base + pg * PAGE;
            c = ch->pc[pg];
            if (c < 0) continue;
            cw = class_words[c];
            n = PAGE / cw;
            for (k = 0; k < n; k++) {
                long *cell = pb + k * cw;
                if (cell[0] && (cell[1] & 1)) {
                    cell[1] &= ~1L;
                    live++;
                } else cell[0] = 0;
            }
            if (!live) {
                /* an empty page: for any size class */
                ch->pc[pg] = -1;
                push_free_page(i * CHUNK_PAGES + pg);
                continue;
            }
            gc_live += live * cw;
            for (k = n - 1; k >= 0; k--) {
                long *cell = pb + k * cw;
                if (!cell[0]) {
                    cell[1] = (long)free_lists[c];
                    free_lists[c] = cell;
                }
            }
        }
    }
    for (i = 0, k = 0; i < nlarges; i++) {
        long *b = larges[i].base;
        if (b[1] & 1) {
            b[1] &= ~1L;
            gc_live += larges[i].words;
            larges[k++] = larges[i];
        } else free(b);
    }
    nlarges = k;
}

/* ------------------------------------------------------------ collecting */

static void gc_collect(void) {
    long here = 0, *p;
    /* not while other threads run: only the main stack is scanned, and the threads are not stopped */
    if (gc_running || !gc_stack_top || thr_active) return;
    gc_running = 1;
    gc_collections++;
    nmark = 0;
    /* the roots: the stack from this frame up, and the program's globals */
    for (p = &here; p <= gc_stack_top; p++) mark_value(*p);
    for (p = gc_gstart; p && p <= gc_gend; p++) mark_value(*p);
    while (nmark) mark_children(mark_stack[--nmark]);
    sweep();
    gc_allocated = 0;
    gc_threshold = gc_live > MIN_THRESHOLD ? gc_live : MIN_THRESHOLD;
    gc_running = 0;
}

void vbrt_gc_collect(void) { gc_collect(); }

/* GC.GetTotalMemory(forceFullCollection): the bytes of the live objects */
long vbrt_gc_total(long force) {
    if (force) gc_collect();
    return (gc_live + gc_allocated) * 8;
}

long vbrt_gc_count_gen(long generation) { return generation == 0 ? gc_collections : 0; }

static long size_class(long need) {
    long c;
    for (c = 0; c < NCLASS; c++) if (class_words[c] >= need) return c;
    return -1;
}

/* an object of n words after its two header words, zeroed; tag: p[-2] */
static long *gc_alloc(long n, long tag) {
    long need = n + 2, c = size_class(need), i;
    long *cell;
    if (gc_stress || gc_allocated > gc_threshold) gc_collect();
    if (c < 0) {
        long *b = calloc(need, sizeof(long)), at;
        if (!b) {
            gc_collect();
            b = calloc(need, sizeof(long));
            if (!b) out_of_memory();
        }
        if (nlarges == cap_larges) {
            cap_larges = cap_larges ? 2 * cap_larges : 64;
            larges = realloc(larges, sizeof(Large) * cap_larges);
        }
        for (at = nlarges; at > 0 && (long)larges[at - 1].base > (long)b; at--) larges[at] = larges[at - 1];
        larges[at].base = b;
        larges[at].words = need;
        nlarges++;
        gc_allocated += need;
        b[0] = tag;
        return b + 2;
    }
    if (!free_lists[c] && !alloc_page(c)) {
        gc_collect();
        if (!free_lists[c] && !alloc_page(c)) out_of_memory();
    }
    cell = free_lists[c];
    free_lists[c] = (long *)cell[1];
    for (i = 0; i < class_words[c]; i++) cell[i] = 0;
    gc_allocated += class_words[c];
    cell[0] = tag;
    return cell + 2;
}

/* vbrt_alloc: gc_alloc, locked while other threads run */
long *vbrt_alloc(long n, long tag) {
    long locked = thr_lock(&thr_alloc_lock);
    long *p = gc_alloc(n, tag);
    if (locked) spinlock_unlock(&thr_alloc_lock);
    return p;
}
