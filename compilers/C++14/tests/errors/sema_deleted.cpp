struct A {
    A() {}
    A(const A &) = delete;
};
int main() {
    A a;
    A b = a;
    return 0;
}
