#include <cstdio>

int add(int a, int b) { return a + b; }
double add(double a, double b) { return a + b; }
int add(int a, int b, int c) { return a + b + c; }
const char *kind(int) { return "int"; }
const char *kind(long) { return "long"; }
const char *kind(double) { return "double"; }
const char *kind(char) { return "char"; }
const char *kind(bool) { return "bool"; }
const char *kind(const char *) { return "string"; }
const char *kind(void *) { return "pointer"; }

int power(int base, int exp = 2) { return exp == 0 ? 1 : base * power(base, exp - 1); }
int scale(int x, int num = 3, int den = 1) { return x * num / den; }

int fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }
inline int sq(int x) { return x * x; }
static int counter() { static int c = 0; return ++c; }

int apply(int (*f)(int), int v) { return f(v); }
int twice(int x) { return 2 * x; }
typedef int (*binop)(int, int);
int fold(binop op, int a, int b) { return op(a, b); }

void swapi(int *a, int *b) { int t = *a; *a = *b; *b = t; }

int main()
{
    printf("%d %.1f %d\n", add(1, 2), add(1.5, 2.5), add(1, 2, 3));
    printf("%s %s %s %s %s %s %s\n", kind(1), kind(1L), kind(1.0), kind('c'), kind(true), kind("s"), kind((void *)0));
    printf("%d %d %d\n", power(3), power(2, 10), power(5, 0));
    printf("%d %d %d\n", scale(10), scale(10, 2), scale(10, 6, 4));
    printf("%d %d\n", fib(10), sq(7));
    counter(); counter();
    printf("%d\n", counter());
    printf("%d %d\n", apply(twice, 21), apply(sq, 5));
    printf("%d %d\n", fold(add, 3, 4), fold(add, 5, 6));
    int x = 1, y = 2;
    swapi(&x, &y);
    printf("%d %d\n", x, y);
    int (*fp)(int) = &twice;
    printf("%d\n", fp(8));
    return 0;
}
