#include "multi_shapes.h"
#include <map>

namespace {
    class Square : public Shape {
        double s_;
    public:
        explicit Square(double s) : s_(s) {}
        double area() const { return s_ * s_; }
        std::string name() const { return "square"; }
    };
    class Circle : public Shape {
        double r_;
    public:
        explicit Circle(double r) : r_(r) {}
        double area() const { return 3.0 * r_ * r_; }
        std::string name() const { return "circle"; }
    };
    std::map<std::string, int> registry = {{"square", 1}, {"circle", 2}};
}

std::vector<std::unique_ptr<Shape>> make_shapes()
{
    std::vector<std::unique_ptr<Shape>> v;
    v.push_back(std::unique_ptr<Shape>(new Square(2)));
    v.push_back(std::unique_ptr<Shape>(new Circle(1)));
    Box<int> b(1);
    (void)b.twice();
    return v;
}

int registry_size() { return (int)registry.size(); }
