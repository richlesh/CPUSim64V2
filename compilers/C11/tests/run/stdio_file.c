#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* stdio file operations.  Positions are counted in characters on text
   streams; the test file is ASCII so this matches native byte offsets. */

static void dump(const char *path) {
    FILE *f = fopen(path, "r");
    int c;
    printf("[");
    while ((c = fgetc(f)) != EOF) putchar(c == '\n' ? '|' : c);
    printf("]\n");
    fclose(f);
}

int main(void) {
    const char *path = "/tmp/cc_stdio_file.txt";
    const char *path2 = "/tmp/cc_stdio_file2.txt";
    char buf[64];
    FILE *f;
    int c;

    /* write, read, append */
    f = fopen(path, "w");
    printf("open w %d\n", f != NULL);
    fputs("line one\n", f);
    fprintf(f, "line %s %d\n", "two", 2);
    fputc('3', f);
    putc('\n', f);
    printf("ftell after write %ld\n", ftell(f));
    fclose(f);
    dump(path);
    f = fopen(path, "a");
    fputs("appended\n", f);
    fclose(f);
    dump(path);

    /* line input, ungetc, feof */
    f = fopen(path, "r");
    while (fgets(buf, sizeof buf, f)) {
        buf[strcspn(buf, "\n")] = 0;
        printf("fgets [%s]\n", buf);
    }
    printf("feof %d ferror %d fgetc %d\n", feof(f), ferror(f), fgetc(f));
    clearerr(f);
    printf("after clearerr feof %d\n", feof(f));
    rewind(f);
    c = fgetc(f);
    int u1 = ungetc('Z', f);
    int g1 = fgetc(f);
    int g2 = fgetc(f);
    printf("first %c ungetc %c then %c %c\n", c, u1, g1, g2);

    /* seeking */
    fseek(f, 5, SEEK_SET);
    c = fgetc(f);
    printf("seek 5: %c ftell %ld\n", c, ftell(f));
    fseek(f, 2, SEEK_CUR);
    printf("seek cur+2: %c\n", fgetc(f));
    fseek(f, -9, SEEK_END);
    fgets(buf, sizeof buf, f);
    long te = ftell(f);
    printf("seek end-9: [%s] ftell %ld\n", strtok(buf, "\n"), te);
    fseek(f, 0, SEEK_SET);
    fpos_t pos;
    fgetc(f);
    fgetc(f);
    fgetpos(f, &pos);
    fgets(buf, 4, f);
    fsetpos(f, &pos);
    c = fgetc(f);
    printf("fgetpos [%s] again %c\n", buf, c);
    printf("small fgets %s", fgets(buf, 3, f) ? buf : "null");
    printf("|\n");
    fclose(f);

    /* getline */
    f = fopen(path, "r");
    char *line = NULL;
    size_t cap = 0;
    ssize_t len;
    int lines = 0;
    while ((len = getline(&line, &cap, f)) != -1) lines++;
    printf("getline lines=%d last len=%d\n", lines, (int)strlen(line));
    free(line);
    fclose(f);

    /* binary read/write with fread/fwrite */
    f = fopen(path2, "wb");
    char out[5] = { 'A', 0, (char)200, '\n', 'z' };
    printf("fwrite %d\n", (int)fwrite(out, 1, 5, f));
    fclose(f);
    f = fopen(path2, "rb");
    char in[8];
    size_t got = fread(in, 1, 8, f);
    /* compare bytes as unsigned: native char is signed */
    printf("fread %d: %d %d %d %d %d eof=%d\n", (int)got, in[0] & 255, in[1] & 255, in[2] & 255,
           in[3] & 255, in[4] & 255, feof(f));
    fseek(f, 2, SEEK_SET);
    printf("bin seek %d\n", fgetc(f));
    fclose(f);

    /* read/write update mode */
    f = fopen(path2, "w+");
    fputs("update", f);
    rewind(f);
    fgets(buf, sizeof buf, f);
    printf("w+ [%s]\n", buf);
    fclose(f);

    /* errors and file management */
    printf("missing %d\n", fopen("/tmp/cc_no_such_dir/x.txt", "r") == NULL);
    printf("bad mode %d\n", fopen(path, "q") == NULL);
    f = fopen(path, "r");
    int wr = fputc('x', f);
    printf("write to read-only %d ferror %d\n", wr, ferror(f) != 0);
    fclose(f);
    printf("rename %d\n", rename(path, path2));
    printf("old gone %d\n", fopen(path, "r") == NULL);
    dump(path2);
    int rm1 = remove(path2);
    int rm2 = remove(path2);
    printf("remove %d %d\n", rm1, rm2 != 0);

    FILE *t = tmpfile();
    fputs("temp data", t);
    rewind(t);
    fgets(buf, sizeof buf, t);
    printf("tmpfile [%s]\n", buf);
    fclose(t);
    char name[L_tmpnam];
    printf("tmpnam %d\n", tmpnam(name) != NULL && name[0] != 0);

    /* standard streams */
    printf("std %d %d %d\n", stdin != NULL, stdout != stderr, fileno(stdout));
    fflush(stdout);
    fflush(NULL);
    puts("puts adds newline");
    fputs("fputs does not", stdout);
    putchar('\n');
    int ch2;
    int count = 0;
    while ((ch2 = getchar()) != EOF) count++;
    printf("stdin chars %d\n", count);
    return 0;
}
