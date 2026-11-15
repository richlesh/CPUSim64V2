#ifndef MULTI_SHAPES_H
#define MULTI_SHAPES_H
#include <string>
#include <vector>
#include <memory>

class Shape {
public:
    virtual ~Shape() {}
    virtual double area() const = 0;
    virtual std::string name() const = 0;
};

std::vector<std::unique_ptr<Shape>> make_shapes();
int registry_size();

template<class T> struct Box {
    T v;
    explicit Box(T x) : v(x) {}
    T twice() const { return v + v; }
};

#endif
