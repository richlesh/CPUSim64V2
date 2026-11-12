#include <stdio.h>
#define N 12
void bubble(int *a, int n) {
    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - 1 - i; j++)
            if (a[j] > a[j + 1]) { int t = a[j]; a[j] = a[j + 1]; a[j + 1] = t; }
}
static void quick(int a[], int lo, int hi) {
    if (lo >= hi) return;
    int p = a[(lo + hi) / 2], i = lo, j = hi;
    while (i <= j) {
        while (a[i] < p) i++;
        while (a[j] > p) j--;
        if (i <= j) { int t = a[i]; a[i] = a[j]; a[j] = t; i++; j--; }
    }
    quick(a, lo, j);
    quick(a, i, hi);
}
void print(const char *label, int *a, int n) {
    printf("%s:", label);
    for (int i = 0; i < n; i++) printf(" %d", a[i]);
    printf("\n");
}
int main(void) {
    int a[N] = { 42, -7, 19, 0, 3, 88, -15, 23, 5, 5, 61, 1 };
    int b[N];
    for (int i = 0; i < N; i++) b[i] = a[N - 1 - i];
    bubble(a, N);
    quick(b, 0, N - 1);
    print("bubble", a, N);
    print("quick", b, N);
    for (int i = 0; i < N; i++) if (a[i] != b[i]) return 1;
    return 0;
}
