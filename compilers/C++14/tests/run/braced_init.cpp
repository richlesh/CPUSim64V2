#include <cstdio>
#include <initializer_list>

struct P { int x, y; };
struct Q { int a; double b; P p; };

struct V {
    int n;
    int sum;
    V(std::initializer_list<int> l) : n((int)l.size()), sum(0) { for (const int *p = l.begin(); p != l.end(); ++p) sum += *p; }
    V(int a, int b) : n(-1), sum(a * b) {}
};

struct Named {
    const char *name;
    int id;
    Named(const char *s, int i) : name(s), id(i) {}
    ~Named() { printf("~%s\n", name); }
};

int total(std::initializer_list<int> l) { int s = 0; for (int v : l) s += v; return s; }
int px(P p) { return p.x; }
int py(const P &p) { return p.y; }
int tn(Named n) { return n.id; }
P make() { return {3, 4}; }
V makev() { return {1, 2, 3}; }
Named mkn() { return {"ret", 9}; }
int plain() { return {42}; }

int main()
{
    P a{1, 2};
    P b = {3, 4};
    Q q{1, 2.5, {5, 6}};
    printf("%d %d %d %g %d\n", a.y, b.x, q.a, q.b, q.p.y);
    V v1{1, 2, 3, 4};
    V v2(3, 4);
    V v3{5, 6};
    V v4 = {7, 8, 9};
    printf("%d %d | %d %d | %d %d | %d %d\n", v1.n, v1.sum, v2.n, v2.sum, v3.n, v3.sum, v4.n, v4.sum);
    printf("%d %d\n", total({1, 2, 3}), total({}));
    printf("%d %d\n", px({7, 8}), py({9, 10}));
    P m = make();
    printf("%d %d\n", m.x, m.y);
    V mv = makev();
    printf("%d %d\n", mv.n, mv.sum);
    printf("%d\n", tn({"tmp", 5}));
    Named r = mkn();
    printf("%d\n", r.id);
    printf("%d\n", plain());
    P arr[2] = {{1, 2}, {3, 4}};
    printf("%d\n", arr[1].x);
    int i{5};
    int z{};
    printf("%d %d\n", i, z);
    P *np = new P{11, 12};
    printf("%d\n", np->y);
    delete np;
    P as;
    as = {20, 21};
    printf("%d %d\n", as.x, as.y);
    return 0;
}
