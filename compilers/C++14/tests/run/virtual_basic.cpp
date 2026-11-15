#include <cstdio>

class Shape {
public:
    Shape(const char *n) : name(n) { printf("Shape(%s)\n", name); }
    virtual ~Shape() { printf("~Shape(%s)\n", name); }
    virtual double area() const { return 0; }
    virtual const char *kind() const { return "shape"; }
    void describe() const { printf("%s %s %.1f\n", name, kind(), area()); }
    virtual void scale(int k) = 0;
protected:
    const char *name;
};

class Rect : public Shape {
public:
    Rect(int w, int h) : Shape("rect"), w_(w), h_(h) {}
    ~Rect() { printf("~Rect\n"); }
    double area() const override { return w_ * h_; }
    const char *kind() const override { return "rect"; }
    void scale(int k) override { w_ *= k; h_ *= k; }
private:
    int w_, h_;
};

class Square : public Rect {
public:
    Square(int s) : Rect(s, s) { printf("Square\n"); }
    ~Square() { printf("~Square\n"); }
    const char *kind() const override { return "square"; }
};

class Circle : public Shape {
    int r;
public:
    Circle(int r) : Shape("circle"), r(r) {}
    double area() const override { return 3 * r * r; }
    void scale(int k) override { r *= k; }
};

void show(const Shape &s) { s.describe(); }

int main()
{
    Shape *shapes[3];
    shapes[0] = new Rect(2, 3);
    shapes[1] = new Square(4);
    shapes[2] = new Circle(2);
    for (int i = 0; i < 3; i++) shapes[i]->describe();
    for (int i = 0; i < 3; i++) shapes[i]->scale(2);
    for (int i = 0; i < 3; i++) show(*shapes[i]);
    for (int i = 0; i < 3; i++) delete shapes[i];
    printf("--\n");
    {
        Square sq(5);
        Shape &r = sq;
        printf("%s %.1f\n", r.kind(), r.area());
    }
    return 0;
}
