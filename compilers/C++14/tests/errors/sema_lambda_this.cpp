struct S {
    int v;
    int f() { auto g = []() { return v; }; return g(); }
};
int main() { S s; return s.f(); }
