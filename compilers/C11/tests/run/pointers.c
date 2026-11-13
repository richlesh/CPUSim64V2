#include <stdio.h>
#include <string.h>
int g = 42;
int garr[5] = { 1, 2, 3 };
int *gp = &g;
int *gp2 = garr + 2;
char gs[] = "global";
char *gmsg = "hello";
int matrix[3][4];
static int counter(void) { static int n = 0; return ++n; }
void swap(int *a, int *b) { int t = *a; *a = *b; *b = t; }
int sum(int *a, int n) { int s = 0; for (int i = 0; i < n; i++) s += a[i]; return s; }
int my_strlen(const char *s) { const char *p = s; while (*p) p++; return p - s; }
void reverse(char *s) { int i = 0, j = strlen(s) - 1; while (i < j) { char c = s[i]; s[i] = s[j]; s[j] = c; i++; j--; } }
int main(void) {
    int x = 1, y = 2;
    swap(&x, &y);
    printf("x=%d y=%d\n", x, y);
    int arr[10];
    for (int i = 0; i < 10; i++) arr[i] = i * i;
    printf("sum=%d\n", sum(arr, 10));
    int *p = arr + 3;
    printf("%d %d %d %d\n", *p, p[2], *(p - 1), (int)(&arr[9] - p));
    p++; p += 2;
    printf("%d\n", *p);
    printf("g=%d *gp=%d *gp2=%d garr[4]=%d\n", g, *gp, *gp2, garr[4]);
    *gp = 7;
    printf("g=%d\n", g);
    printf("%s %s %d %d\n", gs, gmsg, my_strlen(gs), (int)sizeof(gs));
    char buf[20] = "abc";
    printf("buf=%s len=%d last=%d\n", buf, my_strlen(buf), buf[19]);
    char s[] = "stressed";
    reverse(s);
    printf("%s\n", s);
    printf("%c%c\n", "xyz"[1], *"q");
    for (int i = 0; i < 3; i++) for (int j = 0; j < 4; j++) matrix[i][j] = i * 10 + j;
    int (*row)[4] = matrix + 1;
    printf("%d %d %d\n", matrix[2][3], row[0][2], *(*(matrix + 2) + 1));
    int c1 = counter(); int c2 = counter(); int c3 = counter();
    printf("%d %d %d\n", c1, c2, c3);
    int primes[50], np = 0;
    char sieve[100];
    for (int i = 0; i < 100; i++) sieve[i] = 1;
    for (int i = 2; i < 100; i++) if (sieve[i]) { primes[np++] = i; for (int j = i * i; j < 100; j += i) sieve[j] = 0; }
    printf("%d primes, last %d\n", np, primes[np - 1]);
    int **pp = &p;
    **pp = 99;
    printf("%d\n", arr[6]);
    char *words[] = { "alpha", "beta", "gamma" };
    for (int i = 0; i < 3; i++) printf("%s%c", words[i], i < 2 ? ',' : '\n');
    char dst[32];
    strcpy(dst, "con");
    strcat(dst, "cat");
    printf("%s %d %d\n", dst, strcmp(dst, "concat"), strcmp("a", "b") < 0);
    return 0;
}
