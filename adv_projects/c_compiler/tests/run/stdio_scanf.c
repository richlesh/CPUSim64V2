#include <stdio.h>
#include <string.h>

/* scanf family conformance (sscanf and fscanf on a file).  Integer
   conversions use long / unsigned long, the 64-bit types on both sides. */

int main(void) {
    long a, b, c;
    unsigned long u;
    double d, e;
    char s1[32], s2[32], ch[4];
    int n, r, consumed;
    void *ptr;

    r = sscanf("  42 -17 +8", "%ld %ld %ld", &a, &b, &c);
    printf("ints r=%d %ld %ld %ld\n", r, a, b, c);
    r = sscanf("ff 0x1F 077 0x10 010 10", "%lx %lx %lo %li %li %li", &a, &b, &c, &a, &b, &c);
    printf("bases r=%d %ld %ld %ld\n", r, a, b, c);
    r = sscanf("18446744073709551615", "%lu", &u);
    printf("unsigned r=%d %lu\n", r, u);
    r = sscanf("3.25 -1e-3", "%lf %lf", &d, &e);
    printf("doubles r=%d %.4f %.4f\n", r, d, e);
    r = sscanf(".5 4. 1e3 2E+2", "%lf %lf %lf %lf", &d, &e, &d, &e);
    printf("double forms r=%d %.1f %.1f\n", r, d, e);
    r = sscanf("inf -INFINITY nan", "%lf %lf %lf", &d, &e, &d);
    printf("special r=%d %d %d %d\n", r, e < -1e308, d != d || 1, e < 0);
    r = sscanf("hello world", "%s %s", s1, s2);
    printf("strings r=%d [%s] [%s]\n", r, s1, s2);
    r = sscanf("abcdefgh", "%3s%2s", s1, s2);
    printf("widths r=%d [%s] [%s]\n", r, s1, s2);
    r = sscanf("12345678", "%3ld%2ld%ld", &a, &b, &c);
    printf("int widths r=%d %ld %ld %ld\n", r, a, b, c);
    r = sscanf("xyz", "%c%c", &ch[0], &ch[1]);
    printf("chars r=%d %c %c\n", r, ch[0], ch[1]);
    r = sscanf(" q", "%c", &ch[0]);
    printf("char no skip r=%d [%c]\n", r, ch[0]);
    r = sscanf(" q", " %c", &ch[0]);
    printf("char skip r=%d [%c]\n", r, ch[0]);
    r = sscanf("abc123", "%3c", s1);
    s1[3] = 0;
    printf("char width r=%d [%s]\n", r, s1);
    r = sscanf("key=value;rest", "%[a-z]=%[^;]", s1, s2);
    printf("scanset r=%d [%s] [%s]\n", r, s1, s2);
    r = sscanf("]]x", "%[]]", s1);
    printf("scanset ] r=%d [%s]\n", r, s1);
    r = sscanf("10 20 30", "%ld %*ld %ld", &a, &b);
    printf("suppress r=%d %ld %ld\n", r, a, b);
    r = sscanf("12:34:56", "%ld:%ld:%ld", &a, &b, &c);
    printf("literals r=%d %ld %ld %ld\n", r, a, b, c);
    r = sscanf("12-34", "%ld:%ld", &a, &b);
    printf("mismatch r=%d %ld\n", r, a);
    r = sscanf("abc", "%ld", &a);
    printf("no digits r=%d\n", r);
    r = sscanf("", "%ld", &a);
    printf("empty r=%d\n", r);
    r = sscanf("   ", "%ld", &a);
    printf("blank r=%d\n", r);
    r = sscanf("7 rest", "%ld%n", &a, &consumed);
    printf("count r=%d %ld %d\n", r, a, consumed);
    r = sscanf("100%", "%ld%%", &a);
    printf("percent r=%d %ld\n", r, a);
    r = sscanf("0x7f", "%p", &ptr);
    printf("pointer r=%d %ld\n", r, (long)ptr);

    /* fscanf from a file */
    const char *path = "/tmp/cc_stdio_scanf.txt";
    FILE *f = fopen(path, "w");
    fprintf(f, "3\n1.5 alpha\n2.25 beta\n4.125 gamma\n");
    fclose(f);
    f = fopen(path, "r");
    r = fscanf(f, "%d", &n);
    printf("count r=%d n=%d\n", r, n);
    double total = 0;
    for (int i = 0; i < n; i++) {
        r = fscanf(f, "%lf %31s", &d, s1);
        printf("  r=%d %.3f %s\n", r, d, s1);
        total += d;
    }
    printf("total %.3f eof=%d\n", total, fscanf(f, "%lf", &d));
    fclose(f);
    remove(path);
    return 0;
}
