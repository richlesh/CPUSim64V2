struct P { int x; int f(int); };
int g(P *p, int a, int b) {
    int r = a * (b + 1) - p->x << 2 > 3 ? a : b;
    r += (int)a + p->f(r);
    auto l = [&](int z) { return z + r; };
    return l(1);
}
