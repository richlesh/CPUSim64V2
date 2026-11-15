#include <cstdio>

struct A {
    int a, b;
    A(int x, int y) : a(x), b(y) { printf("A(%d,%d)\n", a, b); }
    A(int x) : A(x, x * 2) { printf("A(int)\n"); }
    A() : A(7) { printf("A()\n"); }
    ~A() { printf("~A %d\n", a); }
};

struct V {
    virtual int f() { return 1; }
    V() : V(0) {}
    V(int) {}
    virtual ~V() {}
};
struct W : V {
    int f() { return 2; }
    W() : V() {}
};

int main()
{
    A x;
    A y(3);
    W w;
    V *p = &w;
    printf("%d %d %d\n", x.b, y.b, p->f());
    return 0;
}
