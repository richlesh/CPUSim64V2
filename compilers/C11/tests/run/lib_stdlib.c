#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

static int cmp_int(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    return x < y ? -1 : x > y;
}

static void bye1(void) { printf("atexit handler 1\n"); }
static void bye2(void) { printf("atexit handler 2\n"); }

int main(void) {
    char *end;

    /* integer arithmetic */
    printf("abs %d %ld %lld\n", abs(-7), labs(-8L), llabs(-9LL));
    div_t q = div(-17, 5);
    ldiv_t lq = ldiv(17L, -5L);
    lldiv_t llq = lldiv(-17LL, -5LL);
    printf("div %d %d / %ld %ld / %lld %lld\n", q.quot, q.rem, lq.quot, lq.rem, llq.quot, llq.rem);

    /* numeric conversion */
    printf("atoi %d %d %d\n", atoi("  42"), atoi("-17abc"), atoi("x"));
    printf("atol %ld atoll %lld\n", atol("123456789012"), atoll("-98765432109"));
    long v = strtol("  -0x1fZ", &end, 16);
    printf("strtol hex %ld rest=%s\n", v, end);
    v = strtol("0777", &end, 0);
    printf("strtol oct %ld rest=[%s]\n", v, end);
    v = strtol("0x", &end, 0);
    printf("strtol 0x %ld rest=%s\n", v, end);
    v = strtol("zz", &end, 36);
    printf("strtol b36 %ld\n", v);
    v = strtol("abc", &end, 10);
    printf("strtol none %ld rest=%s\n", v, end);
    printf("strtol max %d min %d\n", strtol("99999999999999999999", 0, 10) == LONG_MAX,
           strtol("-99999999999999999999", 0, 10) == LONG_MIN);
    printf("strtol lmin %d\n", strtol("-9223372036854775808", 0, 10) == LONG_MIN);
    printf("strtoll %lld\n", strtoll("-1234", 0, 10));
    printf("strtoul %d %d %lu\n", strtoul("18446744073709551615", 0, 10) == ULONG_MAX,
           strtoul("99999999999999999999", 0, 10) == ULONG_MAX, strtoul("1000", 0, 2));
    printf("strtoull %llu\n", strtoull("ff", 0, 16));
    double dv = strtod("  -12.5e2xyz", &end);
    printf("strtod %.2f rest=%s\n", dv, end);
    dv = strtod("3.", &end);
    printf("strtod %.2f rest=[%s]\n", dv, end);
    dv = strtod(".5e", &end);
    printf("strtod %.2f rest=%s\n", dv, end);
    dv = strtod("nope", &end);
    printf("strtod %.2f rest=%s\n", dv, end);
    dv = strtod("-Infinity", &end);
    printf("strtod inf %d %d rest=[%s]\n", dv < -1e308, dv < 0, end);
    printf("atof %.3f\n", atof("2.718"));

    /* random numbers: deterministic for a seed, within [0, RAND_MAX] */
    srand((unsigned)time(NULL));          /* seeded from the wall clock */
    int t1 = rand();
    srand((unsigned)time(NULL) + 1);
    int t2 = rand();
    printf("time seeds differ: %d\n", t1 != t2);
    srand(42);
    int r1 = rand(), r2 = rand();
    srand(42);
    int same = rand() == r1 && rand() == r2;
    int inrange = 1, distinct = 0, prev = -1;
    for (int i = 0; i < 1000; i++) {
        int r = rand();
        if (r < 0 || r > RAND_MAX) inrange = 0;
        if (r != prev) distinct++;
        prev = r;
    }
    printf("rand same=%d inrange=%d distinct=%d max=%d\n", same, inrange, distinct > 990, RAND_MAX);

    /* searching and sorting */
    int a[20];
    srand(7);
    for (int i = 0; i < 20; i++) a[i] = (i * 37 + 11) % 23 - 10;
    qsort(a, 20, sizeof a[0], cmp_int);
    for (int i = 0; i < 20; i++) printf("%d ", a[i]);
    printf("\n");
    int sorted = 1;
    for (int i = 1; i < 20; i++) if (a[i - 1] > a[i]) sorted = 0;
    int key = 5, missing = 100;
    int *hit = bsearch(&key, a, 20, sizeof a[0], cmp_int);
    printf("sorted=%d found=%d missing=%d\n", sorted, hit && *hit == 5,
           bsearch(&missing, a, 20, sizeof a[0], cmp_int) == NULL);
    qsort(a, 0, sizeof a[0], cmp_int);
    qsort(a, 1, sizeof a[0], cmp_int);

    /* environment and host */
    printf("getenv=%d\n", getenv("CPUSIM64_NO_SUCH_VARIABLE") == NULL);
    printf("system=%d\n", system("true"));

    /* memory */
    long *m = calloc(5, sizeof *m);
    m = realloc(m, 50 * sizeof *m);
    m[49] = 1;
    printf("calloc %ld %ld\n", m[0] + m[4], m[49]);
    free(m);
    free(NULL);

    /* termination: handlers run in reverse order after main returns */
    int a1 = atexit(bye1);
    int a2 = atexit(bye2);
    printf("atexit %d %d\n", a1, a2);
    printf("main returns\n");
    return 0;
}
