#include <stdio.h>
#include <string.h>

/* Random-access file I/O through stdio: update modes, in-place rewrites,
   switching between reading and writing with fseek, byte positions on
   text streams, seeking past the end, and a fixed-size record file. */

struct rec { long id; long score; };

static void show(const char *path) {
    FILE *f = fopen(path, "rb");
    int c;
    printf("[");
    while ((c = fgetc(f)) != EOF) {
        if (c == '\n') printf("|");
        else if (c == 0) printf("0");
        else if (c >= 32 && c < 127) putchar(c);
        else printf("<%d>", c);
    }
    printf("]\n");
    fclose(f);
}

int main(void) {
    const char *path = "/tmp/cc_stdio_random.txt";
    char buf[64];
    FILE *f;
    long n;
    int c0;

    /* w+: write, rewind, read back, patch in place */
    f = fopen(path, "w+");
    fputs("alpha beta gamma\n", f);
    fputs("second line\n", f);
    printf("size %ld\n", ftell(f));
    rewind(f);
    fgets(buf, sizeof buf, f);
    printf("read [%s]", buf);
    fseek(f, 6, SEEK_SET);
    fputs("BETA", f);
    printf("after patch tell %ld\n", ftell(f));
    fseek(f, 0, SEEK_CUR);
    fgets(buf, sizeof buf, f);
    printf("continue [%s]", buf);
    fclose(f);
    show(path);

    /* r+: overwrite without truncating */
    f = fopen(path, "r+");
    fseek(f, -5, SEEK_END);
    fputs("LINE", f);
    fseek(f, 0, SEEK_END);
    printf("r+ size %ld\n", ftell(f));
    fclose(f);
    show(path);

    /* a+: reads anywhere, writes always append */
    f = fopen(path, "a+");
    fseek(f, 0, SEEK_SET);
    printf("a+ first char %c\n", fgetc(f));
    fputs("third\n", f);
    printf("a+ tell after write %ld\n", ftell(f));
    fseek(f, 0, SEEK_SET);
    n = 0;
    while (fgets(buf, sizeof buf, f)) n++;
    printf("a+ lines %ld\n", n);
    fclose(f);
    show(path);

    /* positions are byte offsets, also for non-ASCII text */
    f = fopen(path, "w+");
    fputs("\u00e9t\u00e9 \u4e16", f);
    printf("utf8 bytes %ld\n", ftell(f));
    fseek(f, 3, SEEK_SET);
    /* (a text char is one codepoint here, one byte natively) */
    c0 = fgetc(f);
    printf("char at byte 3 starts with %s\n", (c0 == 0xe9 || c0 == 0xc3) ? "e-acute" : "?");
    fclose(f);

    /* seeking past the end and writing leaves a zero-filled gap */
    f = fopen(path, "w+b");
    fputs("ab", f);
    printf("seek past end %d\n", fseek(f, 5, SEEK_SET));
    fputc('Z', f);
    printf("gap size %ld\n", ftell(f));
    fclose(f);
    show(path);

    /* fixed-size binary records, read and updated by index */
    f = fopen(path, "w+b");
    for (long i = 0; i < 5; i++) {
        struct rec r = { i, i * 10 };   /* binary chars hold bytes: keep values < 256 */
        fwrite(&r, sizeof r, 1, f);
    }
    struct rec r;
    fseek(f, 3 * (long)sizeof r, SEEK_SET);
    fread(&r, sizeof r, 1, f);
    printf("record 3: id=%ld score=%ld\n", r.id, r.score);
    r.score = 99;
    fseek(f, -(long)sizeof r, SEEK_CUR);
    fwrite(&r, sizeof r, 1, f);
    rewind(f);
    long total = 0;
    while (fread(&r, sizeof r, 1, f) == 1) total += r.score;
    printf("total %ld records %ld\n", total, ftell(f) / (long)sizeof r);
    fclose(f);

    /* ungetc and ftell; seek errors */
    f = fopen(path, "r");
    fgetc(f);
    long before = ftell(f);
    int c = fgetc(f);
    ungetc(c, f);
    printf("ungetc tell %d\n", ftell(f) == before);
    printf("seek before start %d\n", fseek(f, -1, SEEK_SET) != 0);
    printf("seek bad whence %d\n", fseek(f, 0, 42) != 0);
    /* (seeking stdout is not tested: natively it depends on where stdout goes) */
    fclose(f);
    remove(path);
    return 0;
}
