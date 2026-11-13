#include <stdio.h>
#include <string.h>
int main(int argc, char **argv) {
    printf("argc=%d\n", argc);
    for (int i = 1; i < argc; i++) printf("argv[%d]=%s len=%d\n", i, argv[i], (int)strlen(argv[i]));
    const char *path = "/tmp/cc_fileio_test.txt";
    FILE *f = fopen(path, "w");
    if (!f) { fprintf(stderr, "cannot open\n"); return 1; }
    fprintf(f, "line %d\n", 1);
    fputs("second line\n", f);
    fputc('3', f);
    fputc('\n', f);
    fclose(f);
    f = fopen(path, "r");
    char buf[100];
    int n = 0;
    while (fgets(buf, sizeof buf, f)) {
        n++;
        buf[strlen(buf) - 1] = 0;
        printf("%d: [%s]\n", n, buf);
    }
    fclose(f);
    printf("missing=%d\n", fopen("/tmp/definitely/not/here.txt", "r") == NULL);
    int c, count = 0, lines = 0;
    while ((c = getchar()) != EOF) { count++; if (c == '\n') lines++; }
    printf("stdin: %d chars %d lines\n", count, lines);
    return argc;
}
