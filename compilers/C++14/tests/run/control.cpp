#include <cstdio>

int classify(int n)
{
    if (n < 0) return -1;
    else if (n == 0) return 0;
    else return 1;
}

int main()
{
    for (int i = -1; i <= 1; i++) printf("%d ", classify(i));
    printf("\n");
    int sum = 0;
    for (int i = 1; i <= 10; ++i) {
        if (i % 2 == 0) continue;
        if (i > 7) break;
        sum += i;
    }
    printf("%d\n", sum);
    int n = 5, fact = 1;
    while (n > 1) fact *= n--;
    printf("%d\n", fact);
    int k = 0;
    do { k += 3; } while (k < 10);
    printf("%d\n", k);
    for (int d = 0; d < 6; d++) {
        switch (d) {
        case 0: printf("zero "); break;
        case 1:
        case 2: printf("small "); break;
        case 3: printf("three ");
        case 4: printf("four-ish "); break;
        default: printf("big ");
        }
    }
    printf("\n");
    int arr[5] = {5, 4, 3, 2, 1};
    int total = 0;
    for (int x : arr) total += x * x;
    printf("%d\n", total);
    for (int &x : arr) x *= 2;
    for (auto x : arr) printf("%d ", x);
    printf("\n");
    int j = 0;
loop:
    if (j < 3) { printf("j%d ", j); j++; goto loop; }
    printf("\n");
    for (int a = 0, b = 10; a < b; a += 3, b -= 3) printf("(%d,%d)", a, b);
    printf("\n");
    for (;;) { if (++k > 12) break; }
    printf("%d\n", k);
    if (int t = k * 2) printf("t=%d\n", t);
    return 0;
}
