#include <cstdio>
#include <stdexcept>
#include <string>

struct Tracer {
    const char *name;
    Tracer(const char *n) : name(n) { printf("+%s\n", name); }
    ~Tracer() { printf("-%s\n", name); }
};

struct MyError {
    int code;
    MyError(int c) : code(c) { printf("MyError(%d)\n", c); }
    MyError(const MyError &o) : code(o.code) { printf("MyError copy\n"); }
    ~MyError() { printf("~MyError\n"); }
};

struct Base { virtual ~Base() {} virtual const char *id() const { return "Base"; } };
struct Derived : Base { const char *id() const { return "Derived"; } };

void thrower(int depth)
{
    Tracer t("t");
    if (depth == 0) throw MyError(7);
    thrower(depth - 1);
    printf("not reached\n");
}

int by_value()
{
    try {
        throw 42;
    } catch (int v) {
        return v + 1;
    }
    return 0;
}

struct Owner {
    Tracer a, b;
    Owner(bool fail) : a("a"), b("b") {
        printf("Owner ctor body\n");
        if (fail) throw std::runtime_error("owner failed");
    }
    ~Owner() { printf("~Owner\n"); }
};

int main()
{
    try {
        thrower(2);
    } catch (const MyError &e) {
        printf("caught %d\n", e.code);
    }
    printf("%d\n", by_value());
    try {
        Tracer outer("outer");
        try {
            Tracer inner("inner");
            throw std::string("text");
        } catch (const int &) {
            printf("wrong\n");
        }
    } catch (const std::string &s) {
        printf("string %s\n", s.c_str());
    }
    try {
        throw std::out_of_range("range");
    } catch (const std::logic_error &e) {
        printf("logic %s\n", e.what());
    }
    try {
        try {
            throw MyError(1);
        } catch (MyError &e) {
            printf("inner caught %d\n", e.code);
            throw;
        }
    } catch (const MyError &e) {
        printf("outer caught %d\n", e.code);
    }
    try {
        Owner o(true);
    } catch (const std::exception &e) {
        printf("owner: %s\n", e.what());
    }
    try {
        throw Derived();
    } catch (const Base &b) {
        printf("base ref: %s\n", b.id());
    }
    try {
        throw "c string";
    } catch (const char *s) {
        printf("%s\n", s);
    }
    try {
        throw 3.5;
    } catch (int) {
        printf("int\n");
    } catch (...) {
        printf("other\n");
    }
    for (int i = 0; i < 3; i++) {
        try {
            if (i == 1) throw i;
            printf("i=%d\n", i);
        } catch (int k) {
            printf("caught %d\n", k);
            continue;
        }
    }
    Tracer last("last");
    return 0;
}
