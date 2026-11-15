#include <cstdio>

struct Drawable {
    virtual ~Drawable() { printf("~Drawable\n"); }
    virtual void draw() const { printf("Drawable::draw\n"); }
    virtual int id() const { return 1; }
};

struct Serializable {
    virtual ~Serializable() { printf("~Serializable\n"); }
    virtual int save() const { return 10; }
    virtual int id() const { return 2; }
};

struct Widget : Drawable, Serializable {
    int v = 5;
    ~Widget() { printf("~Widget\n"); }
    void draw() const override { printf("Widget::draw %d\n", v); }
    int save() const override { return v * 100; }
    int id() const override { return 3; }
};

struct Plain { int p = 9; };
struct Mixed : Plain, Drawable {
    void draw() const override { printf("Mixed::draw %d\n", p); }
};

int main()
{
    Widget w;
    Drawable *d = &w;
    Serializable *s = &w;
    d->draw();
    printf("%d %d %d\n", s->save(), d->id(), s->id());
    printf("%d\n", w.id());
    delete (new Widget);
    Drawable *heap = new Widget;
    delete heap;
    Serializable *heap2 = new Widget;
    delete heap2;
    Mixed m;
    Drawable *dm = &m;
    dm->draw();
    printf("--\n");
    return 0;
}
