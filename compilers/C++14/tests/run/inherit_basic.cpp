#include <cstdio>

struct Animal {
    int legs;
    Animal(int l = 4) : legs(l) { printf("Animal(%d)\n", legs); }
    ~Animal() { printf("~Animal\n"); }
    int count() const { return legs; }
    const char *name() const { return "animal"; }
protected:
    int secret = 7;
};

struct Dog : Animal {
    int tricks;
    Dog() : Animal(4), tricks(2) { printf("Dog\n"); }
    Dog(int t) : tricks(t) { printf("Dog(%d)\n", t); }
    ~Dog() { printf("~Dog\n"); }
    const char *name() const { return "dog"; }
    int peek() const { return secret + legs; }
};

struct Puppy : public Dog {
    Puppy() : Dog(1) { printf("Puppy\n"); }
    ~Puppy() { printf("~Puppy\n"); }
    int total() const { return tricks + legs + Animal::count(); }
};

int legs_of(const Animal &a) { return a.legs; }
int legs_ptr(Animal *a) { return a->count(); }
int by_value(Animal a) { return a.legs; }

class Base1 { public: int a; Base1() : a(1) {} int fa() { return a; } };
class Base2 { public: int b; Base2() : b(2) {} int fb() { return b; } };
class Multi : public Base1, public Base2 {
public:
    int c;
    Multi() : c(3) {}
    int sum() { return a + b + c; }
};

int use1(Base1 *p) { return p->fa(); }
int use2(Base2 *p) { return p->fb(); }

int main()
{
    {
        Dog d;
        printf("%d %d %s %s\n", d.legs, d.tricks, d.name(), d.Animal::name());
        printf("%d\n", d.peek());
        printf("%d %d\n", legs_of(d), legs_ptr(&d));
        printf("%d\n", by_value(d));
        Animal &ar = d;
        Animal *ap = &d;
        printf("%d %d %s\n", ar.legs, ap->count(), ap->name());
        Dog *dp = static_cast<Dog *>(ap);
        printf("%d\n", dp->tricks);
    }
    printf("--\n");
    {
        Puppy p;
        printf("%d\n", p.total());
    }
    printf("--\n");
    Multi m;
    printf("%d %d %d\n", m.sum(), use1(&m), use2(&m));
    Base2 *b2 = &m;
    Multi *back = static_cast<Multi *>(b2);
    printf("%d %d\n", b2->b, back->c);
    Animal copy = Dog(9);
    printf("%d\n", copy.legs);
    return 0;
}
