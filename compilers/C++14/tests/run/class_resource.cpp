#include <cstdio>
#include <cstring>

class IntArray {
    int *data;
    int n;
public:
    IntArray() : data(nullptr), n(0) { printf("default\n"); }
    explicit IntArray(int size) : data(new int[size]), n(size) {
        for (int i = 0; i < n; i++) data[i] = 0;
        printf("alloc %d\n", n);
    }
    IntArray(const IntArray &o) : data(new int[o.n]), n(o.n) {
        for (int i = 0; i < n; i++) data[i] = o.data[i];
        printf("copy %d\n", n);
    }
    IntArray &operator=(const IntArray &o) {
        if (this != &o) {
            delete[] data;
            n = o.n;
            data = new int[n];
            for (int i = 0; i < n; i++) data[i] = o.data[i];
        }
        printf("assign %d\n", n);
        return *this;
    }
    ~IntArray() { printf("free %d\n", n); delete[] data; }
    int &operator[](int i) { return data[i]; }
    int size() const { return n; }
    int sum() const { int s = 0; for (int i = 0; i < n; i++) s += data[i]; return s; }
};

IntArray make(int n)
{
    IntArray a(n);
    for (int i = 0; i < n; i++) a[i] = i + 1;
    return a;
}

struct Tracker {
    int id;
    Tracker(int i) : id(i) { printf("T%d\n", id); }
    ~Tracker() { printf("~T%d\n", id); }
    bool ok() const { return id > 0; }
};

bool test(int x) { return x > 0; }

class Outer {
    int secret = 42;
public:
    class Inner {
    public:
        int peek(const Outer &o) { return o.secret; }
    };
    int get() const { return secret; }
};

class Greeter {
public:
    Greeter &say(const char *s) { printf("%s ", s); return *this; }
    Greeter &end() { printf("\n"); return *this; }
    int calls(int a, int b = 2) const { return a * b; }
    int calls(int a) { return a; }
};

int main()
{
    IntArray a = make(4);
    printf("%d %d\n", a.size(), a.sum());
    IntArray b;
    b = a;
    b[0] = 100;
    printf("%d %d\n", a.sum(), b.sum());
    IntArray c(b);
    printf("%d\n", c[0]);
    printf("-- conditional temporaries\n");
    if (test(1) && Tracker(1).ok()) printf("both\n");
    if (test(0) && Tracker(2).ok()) printf("never\n");
    int r = test(1) ? Tracker(3).id : Tracker(4).id;
    printf("%d\n", r);
    printf("-- inner\n");
    Outer o;
    Outer::Inner in;
    printf("%d %d\n", in.peek(o), o.get());
    Greeter g;
    g.say("hello").say("world").end();
    const Greeter cg;
    printf("%d %d\n", cg.calls(3), g.calls(3));
    return 0;
}
