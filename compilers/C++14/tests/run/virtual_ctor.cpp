#include <cstdio>

struct B {
    B() { printf("B ctor: "); who(); }
    virtual ~B() { printf("B dtor: "); who(); }
    virtual void who() const { printf("B\n"); }
    void call() const { who(); }
};

struct D : B {
    D() { printf("D ctor: "); who(); }
    ~D() { printf("D dtor: "); who(); }
    void who() const override { printf("D\n"); }
};

struct Final final : D {
    void who() const override { printf("Final\n"); }
};

struct Counter {
    virtual int next() { return ++n; }
    int n = 0;
};
struct Doubler : Counter {
    int next() override { return Counter::next() * 2; }
};

int main()
{
    D d;
    d.call();
    B *p = &d;
    p->call();
    Final f;
    f.call();
    printf("--\n");
    Doubler dd;
    Counter &c = dd;
    int first = c.next();
    int second = c.next();
    printf("%d %d\n", first, second);
    return 0;
}
