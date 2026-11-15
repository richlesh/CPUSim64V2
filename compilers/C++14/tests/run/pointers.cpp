#include <cstdio>
#include <cstring>
#include <cstdlib>

int sum(const int *a, int n) { int s = 0; for (int i = 0; i < n; i++) s += a[i]; return s; }
int length(const char *s) { int n = 0; while (*s++) n++; return n; }
void reverse(char *s)
{
    int n = (int)strlen(s);
    for (int i = 0; i < n / 2; i++) { char t = s[i]; s[i] = s[n - 1 - i]; s[n - 1 - i] = t; }
}

int main()
{
    int a[5] = {1, 2, 3, 4, 5};
    int *p = a;
    printf("%d %d %d\n", *p, *(p + 2), p[4]);
    p += 2;
    printf("%d %d\n", *p, (int)(p - a));
    printf("%d\n", sum(a, 5));
    int m[2][3] = {{1, 2, 3}, {4, 5, 6}};
    int t = 0;
    for (int i = 0; i < 2; i++) for (int j = 0; j < 3; j++) t += m[i][j] * (i + 1);
    printf("%d\n", t);
    char s[16] = "hello";
    printf("%s %d %d\n", s, length(s), (int)strlen(s));
    reverse(s);
    printf("%s\n", s);
    char buf[32];
    strcpy(buf, "ab");
    strcat(buf, "cd");
    printf("%s %d\n", buf, strcmp(buf, "abcd"));
    const char *names[] = {"x", "yy", "zzz"};
    for (int i = 0; i < 3; i++) printf("%s=%d ", names[i], length(names[i]));
    printf("\n");
    void *vp = &a[1];
    int *ip = (int *)vp;
    printf("%d\n", *ip);
    int *np = nullptr;
    printf("%d %d\n", np == nullptr, p != nullptr);
    int **pp = &p;
    printf("%d\n", **pp);
    char text[] = "abc";
    char *tp = text;
    *tp = 'X';
    printf("%s\n", text);
    return 0;
}
