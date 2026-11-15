#include <cstdio>
#include <typeinfo>
#include <string>

struct Shape { virtual ~Shape() {} virtual int sides() const { return 0; } };
struct Circle : Shape { int sides() const { return 1; } };
struct Rect : Shape { int sides() const { return 4; } int w = 2; };
struct Square : Rect { int sides() const { return 4; } };

struct A { virtual ~A() {} int a = 1; };
struct B { virtual ~B() {} int b = 2; };
struct AB : A, B { int ab = 3; };

struct Plain { int x; };

int main()
{
    Circle c;
    Rect r;
    Square s;
    Shape *sp[3] = {&c, &r, &s};
    for (int i = 0; i < 3; i++) {
        Shape *p = sp[i];
        Rect *rp = dynamic_cast<Rect *>(p);
        Square *sq = dynamic_cast<Square *>(p);
        Circle *cp = dynamic_cast<Circle *>(p);
        printf("%d: rect=%d square=%d circle=%d\n", i, rp != 0, sq != 0, cp != 0);
    }
    printf("%d %d\n", typeid(c) == typeid(Circle), typeid(*sp[1]) == typeid(Rect));
    printf("%d %d\n", typeid(*sp[2]) == typeid(Rect), typeid(*sp[2]) != typeid(Shape));
    printf("%d\n", typeid(int) == typeid(int));
    printf("%d\n", typeid(int) == typeid(long));
    try {
        Shape &ref = c;
        Rect &rr = dynamic_cast<Rect &>(ref);
        printf("no throw %d\n", rr.w);
    } catch (const std::bad_cast &e) {
        printf("bad_cast\n");
    }
    Shape &rs = s;
    Rect &ok = dynamic_cast<Rect &>(rs);
    printf("%d\n", ok.w);
    AB ab;
    A *ap = &ab;
    B *bp = dynamic_cast<B *>(ap);
    AB *abp = dynamic_cast<AB *>(bp);
    printf("%d %d %d\n", bp != 0 && bp->b == 2, abp == &ab, dynamic_cast<A *>(bp) == ap);
    void *whole = dynamic_cast<void *>(bp);
    printf("%d\n", whole == (void *)&ab);
    Shape *np = 0;
    printf("%d\n", dynamic_cast<Rect *>(np) == 0);
    const Shape *cs = &r;
    const Rect *crp = dynamic_cast<const Rect *>(cs);
    printf("%d\n", crp != 0);
    printf("%d\n", typeid(Rect).before(typeid(Square)) || typeid(Square).before(typeid(Rect)));
    return 0;
}
