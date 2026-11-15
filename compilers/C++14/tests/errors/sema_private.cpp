class A {
    int secret;
public:
    A() : secret(1) {}
};
int main() {
    A a;
    return a.secret;
}
