// A program of three units: multi_main.cpp, multi_shapes.cpp and multi_util.cpp.
#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>
#include "multi_shapes.h"

int checked_div(int a, int b);
extern int util_calls;

int main()
{
    std::vector<std::unique_ptr<Shape>> shapes = make_shapes();
    double total = 0;
    for (const auto &s : shapes) {
        std::cout << s->name() << " " << s->area() << "\n";
        total += s->area();
    }
    std::cout << "total " << total << "\n";
    std::cout << checked_div(10, 3) << "\n";
    try {
        checked_div(1, 0);
    } catch (const std::runtime_error &e) {
        std::cout << "caught: " << e.what() << "\n";
    }
    std::cout << "calls " << util_calls << " registry " << registry_size() << "\n";
    Box<int> b(5);
    std::cout << b.twice() << "\n";
    return 0;
}
