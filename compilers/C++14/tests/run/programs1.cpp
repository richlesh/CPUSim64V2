// small classic programs in the non-class subset
#include <cstdio>
#include <cstring>
#include <cstdlib>

typedef int (*cmp_fn)(const void *, const void *);

static int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return x < y ? -1 : (x > y ? 1 : 0);
}

int gcd(int a, int b) { return b == 0 ? a : gcd(b, a % b); }

bool is_prime(int n)
{
    if (n < 2) return false;
    for (int i = 2; i * i <= n; i++)
        if (n % i == 0) return false;
    return true;
}

void bubble(int *a, int n)
{
    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - 1 - i; j++)
            if (a[j] > a[j + 1]) { int t = a[j]; a[j] = a[j + 1]; a[j + 1] = t; }
}

void matmul(const int a[2][2], const int b[2][2], int c[2][2])
{
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++) {
            c[i][j] = 0;
            for (int k = 0; k < 2; k++) c[i][j] += a[i][k] * b[k][j];
        }
}

int count_words(const char *s)
{
    int n = 0;
    bool in = false;
    for (; *s; s++) {
        if (*s == ' ') in = false;
        else if (!in) { in = true; n++; }
    }
    return n;
}

unsigned long long fact(unsigned n) { return n <= 1 ? 1ULL : n * fact(n - 1); }

int main(int argc, char **argv)
{
    printf("%d %d\n", gcd(48, 18), gcd(17, 5));
    int primes = 0;
    for (int i = 0; i < 50; i++) if (is_prime(i)) primes++;
    printf("%d\n", primes);
    int arr[8] = {5, 2, 9, 1, 7, 3, 8, 4};
    bubble(arr, 8);
    for (int i = 0; i < 8; i++) printf("%d ", arr[i]);
    printf("\n");
    int arr2[6] = {9, 4, 7, 1, 8, 2};
    qsort(arr2, 6, sizeof(int), cmp_int);
    for (int i = 0; i < 6; i++) printf("%d ", arr2[i]);
    printf("\n");
    int a[2][2] = {{1, 2}, {3, 4}}, b[2][2] = {{5, 6}, {7, 8}}, c[2][2];
    matmul(a, b, c);
    printf("%d %d %d %d\n", c[0][0], c[0][1], c[1][0], c[1][1]);
    printf("%d\n", count_words("the quick  brown fox"));
    printf("%llu\n", fact(15));
    printf("%d\n", argc);
    char word[20];
    const char *src = "Hello, World";
    int n = 0;
    for (const char *p = src; *p; p++) if (*p >= 'a' && *p <= 'z') word[n++] = *p - 32; else word[n++] = *p;
    word[n] = '\0';
    printf("%s\n", word);
    int neg = -7;
    printf("%d %d\n", neg / 2, neg % 3);
    return 0;
}
