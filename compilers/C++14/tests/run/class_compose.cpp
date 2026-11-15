#include <cstdio>

struct Part {
    int v;
    Part(int x = 0) : v(x) { printf("Part(%d)\n", v); }
    Part(const Part &o) : v(o.v) { printf("Part copy %d\n", v); }
    Part &operator=(const Part &o) { printf("Part assign %d<-%d\n", v, o.v); v = o.v; return *this; }
    ~Part() { printf("~Part(%d)\n", v); }
};

struct Whole {
    Part first;
    Part second;
    int tag;
    Whole(int a, int b) : first(a), second(b), tag(a + b) { printf("Whole(%d)\n", tag); }
    // implicit copy constructor, assignment and destructor
};

struct Plain { int a; double b; char name[8]; };

struct Arr {
    Part items[2];
    Arr() { printf("Arr body\n"); }
};

struct WithRef {
    int &r;
    const int c;
    WithRef(int &x) : r(x), c(5) {}
    int sum() const { return r + c; }
};

union Bits { int i; double d; };

struct Counter {
    static int live;
    Counter() { ++live; }
    Counter(const Counter &) { ++live; }
    ~Counter() { --live; }
};
int Counter::live = 0;

Part global_part(99);

Whole make_whole(int n) { return Whole(n, n * 2); }

int main()
{
    printf("-- composition\n");
    {
        Whole w(1, 2);
        printf("-- copy\n");
        Whole w2 = w;
        printf("-- assign\n");
        w2 = Whole(5, 6);
        printf("-- end scope\n");
    }
    printf("-- plain\n");
    Plain p1 = {1, 2.5, "ab"};
    Plain p2 = p1;
    p2.a = 7;
    printf("%d %d %.1f %s\n", p1.a, p2.a, p2.b, p2.name);
    printf("-- array member\n");
    { Arr a; }
    printf("-- ref member\n");
    int base = 10;
    WithRef wr(base);
    base = 20;
    printf("%d\n", wr.sum());
    printf("-- union\n");
    Bits b;
    b.i = 65;
    printf("%d\n", b.i);
    printf("-- counter\n");
    {
        Counter c1;
        Counter c2 = c1;
        Counter arr[3];
        printf("%d\n", Counter::live);
    }
    printf("%d\n", Counter::live);
    printf("-- return\n");
    Whole w3 = make_whole(3);
    printf("%d\n", w3.tag);
    printf("-- static local\n");
    for (int i = 0; i < 2; i++) { static Part sp(7); printf("loop %d\n", sp.v); }
    printf("-- end\n");
    return 0;
}
