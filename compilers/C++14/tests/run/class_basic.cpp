#include <cstdio>

struct Point {
    int x, y;
    int sum() const { return x + y; }
    void move(int dx, int dy) { x += dx; y += dy; }
    static int origin_count;
    static int make(int a) { return a * 2; }
};
int Point::origin_count = 3;

class Counter {
public:
    void inc() { ++n; }
    int get() const { return n; }
    int twice() const { return get() * 2; }
private:
    int n = 0;
};

struct Pair { int a; double b; const char *name; };

int use(Point p) { return p.x * 10 + p.y; }
int use_ref(const Point &p) { return p.sum(); }
void modify(Point &p) { p.x = 100; }
Point make_point(int a, int b) { Point p; p.x = a; p.y = b; return p; }

int main()
{
    Point p;
    p.x = 3;
    p.y = 4;
    printf("%d\n", p.sum());
    p.move(1, 2);
    printf("%d %d\n", p.x, p.y);
    Point *pp = &p;
    pp->x = 9;
    printf("%d %d\n", pp->x, pp->sum());
    printf("%d %d\n", Point::origin_count, Point::make(21));
    Counter c;
    c.inc();
    c.inc();
    printf("%d %d\n", c.get(), c.twice());
    Pair pr = {1, 2.5, "pair"};
    printf("%d %.1f %s\n", pr.a, pr.b, pr.name);
    Point q = {5, 6};
    printf("%d %d\n", use(q), use_ref(q));
    modify(q);
    printf("%d\n", q.x);
    Point r = make_point(7, 8);
    printf("%d %d\n", r.x, r.y);
    Point s = r;
    s.x = 1;
    printf("%d %d\n", r.x, s.x);
    Point arr[3];
    for (int i = 0; i < 3; i++) { arr[i].x = i; arr[i].y = i * i; }
    printf("%d\n", arr[2].sum());
    return 0;
}
