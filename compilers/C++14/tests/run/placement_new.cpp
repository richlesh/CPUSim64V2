#include <cstdio>
#include <cstdlib>

inline void *operator new(unsigned long, void *p) { return p; }

struct T {
    int v;
    T(int x) : v(x) { printf("T(%d)\n", v); }
    ~T() { printf("~T(%d)\n", v); }
};

struct Agg { int a; int b[3]; };

template<class X> void destroy(X *p) { p->~X(); }

int main()
{
    void *mem = malloc(sizeof(T) * 2);
    T *t0 = new (mem) T(1);
    T *t1 = new ((T *)mem + 1) T(2);
    printf("%d %d\n", t0->v, t1->v);
    t1->~T();
    destroy(t0);
    int *ip = new (malloc(sizeof(int))) int(7);
    typedef int I;
    ip->~I();
    printf("%d\n", *ip);
    free(ip);
    free(mem);
    Agg a = {1, 2, 3, 4};
    printf("%d %d %d %d\n", a.a, a.b[0], a.b[1], a.b[2]);
    struct W { int x[2]; int y; };
    W w = {5, 6, 7};
    printf("%d %d %d\n", w.x[0], w.x[1], w.y);
    return 0;
}
