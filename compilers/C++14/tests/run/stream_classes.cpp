#include <iostream>
#include <string>

class Point {
    int x_, y_;
public:
    Point(int x = 0, int y = 0) : x_(x), y_(y) {}
    friend std::ostream &operator<<(std::ostream &os, const Point &p)
    {
        return os << "(" << p.x_ << ", " << p.y_ << ")";
    }
    friend std::istream &operator>>(std::istream &is, Point &p)
    {
        return is >> p.x_ >> p.y_;
    }
    Point operator+(const Point &o) const { return Point(x_ + o.x_, y_ + o.y_); }
};

namespace shapes {
    struct Circle { double r; };
    std::ostream &operator<<(std::ostream &os, const Circle &c) { return os << "Circle(r=" << c.r << ")"; }
}

class Person {
    std::string name_;
    int age_;
public:
    Person(const std::string &n, int a) : name_(n), age_(a) {}
    const std::string &name() const { return name_; }
    int age() const { return age_; }
};

std::ostream &operator<<(std::ostream &os, const Person &p)
{
    os << p.name() << " (" << p.age() << ")";
    return os;
}

int main()
{
    Point a(1, 2), b(3, 4);
    std::cout << a << " + " << b << " = " << a + b << std::endl;
    Point c;
    std::cin >> c;
    std::cout << "read " << c << std::endl;
    shapes::Circle ci = {2.5};
    std::cout << ci << std::endl;
    Person p("Ada", 36);
    std::cout << p << '\n';
    std::string name;
    std::cout << "name? ";
    std::cin >> name;
    std::cout << "Hello, " << name << "!\n";
    std::string line;
    std::getline(std::cin, line);
    std::getline(std::cin, line);
    std::cout << "[" << line << "]\n";
    return 0;
}
