struct V { int x; V(int x = 0) : x(x) {} };
V operator+(const V &a, const V &b) { return V(a.x + b.x); }
V operator-(const V &a) { return V(-a.x); }
bool operator<(const V &a, const V &b) { return a.x < b.x; }
V &operator+=(V &a, const V &b) { a.x += b.x; return a; }
V operator"" _v(unsigned long long n) { return V((int)n); }
struct Fn { int operator()(int a) const { return a; } int operator()(int a, int b) const { return a + b; } };
struct Conv { operator V() const { return V(1); } operator int *() const { return nullptr; } };
int (*get_fp(int sel))(int);
int apply(int (*f)(int), int v) { return f(v); }
int (*table[3])(int);
typedef int (Fn::*MemFn)(int) const;
int callm(Fn &f, MemFn m) { return (f.*m)(1); }
int test() {
    V a(1), b(2), c;
    c = a + b - -a;
    c += b;
    bool lt = a < b;
    Fn fn;
    int r = fn(1) + fn(1, 2) + callm(fn, &Fn::operator());
    V lit = 5_v;
    int i = 1, j = 2;
    int *ip = &i;
    int **ipp = &ip;
    int &ref = **ipp;
    i = (j, i);
    i = j = 3;
    i = i > j ? i : j;
    char buf[8] = "abc";
    const char *names[] = {"a", "b"};
    unsigned char uc = 255;
    long long ll = -1LL;
    unsigned long ul = 1UL;
    double d = 1e10, e = .5, f = 1., g = 0x1p3;
    i = sizeof buf / sizeof buf[0];
    i = (int)d + int(e);
    i = *ip++ + ++*ip;
    switch (i) { case 1: { break; } default: break; }
    return r + (int)lt + lit.x + ref;
}
void labels() { int i = 0; again: i++; if (i < 3) goto again; }
