#include <cstdio>

struct Trace {
    int id;
    Trace() : id(0) { printf("ctor %d\n", id); }
    Trace(int i) : id(i) { printf("ctor %d\n", id); }
    Trace(const Trace &o) : id(o.id + 100) { printf("copy %d from %d\n", id, o.id); }
    Trace &operator=(const Trace &o) { printf("assign %d = %d\n", id, o.id); id = o.id; return *this; }
    ~Trace() { printf("dtor %d\n", id); }
};

struct Holder {
    Trace a;
    Trace b;
    int n = 5;
    Holder() : b(2) { printf("Holder body %d\n", n); }
    ~Holder() { printf("~Holder\n"); }
};

Trace make(int i) { Trace t(i); return t; }
Trace make2(int i) { return Trace(i); }
void by_value(Trace t) { printf("by_value %d\n", t.id); }
void by_ref(const Trace &t) { printf("by_ref %d\n", t.id); }

Trace global_t(77);

int main()
{
    printf("start\n");
    {
        Trace a;
        Trace b(1);
        Trace c = b;
        Trace d(c);
        printf("in scope\n");
    }
    printf("--- holder\n");
    { Holder h; }
    printf("--- calls\n");
    by_value(Trace(5));
    Trace t(6);
    by_value(t);
    by_ref(t);
    by_ref(Trace(7));
    printf("--- returns\n");
    Trace m = make(8);
    Trace m2 = make2(9);
    printf("--- assign\n");
    m = m2;
    printf("--- temporaries\n");
    printf("%d\n", Trace(10).id + Trace(11).id);
    make(12);
    printf("--- loop\n");
    for (int i = 0; i < 2; i++) {
        Trace x(20 + i);
        if (i == 0) continue;
        printf("after continue\n");
    }
    printf("--- new\n");
    Trace *p = new Trace(30);
    delete p;
    Trace *arr = new Trace[2];
    delete[] arr;
    Trace local_arr[2];
    printf("--- end\n");
    return 0;
}
