#include <cstdio>
#include <cstdlib>

int apply(int (*f)(int), int x) { return f(x); }
int cmp_desc(const void *a, const void *b) { return *(const int *)b - *(const int *)a; }

int main()
{
    int (*sq)(int) = [](int x) { return x * x; };
    printf("%d\n", sq(7));
    printf("%d\n", apply([](int v) { return v + 1; }, 41));
    int a[5] = {3, 1, 4, 1, 5};
    qsort(a, 5, sizeof(int), [](const void *x, const void *y) -> int { return *(const int *)x - *(const int *)y; });
    for (int i = 0; i < 5; i++) printf("%d ", a[i]);
    printf("\n");
    void (*hello)() = []() { printf("hello\n"); };
    hello();
    return 0;
}
