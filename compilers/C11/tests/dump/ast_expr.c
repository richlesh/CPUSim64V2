int a, b, c, *p, **pp;
double d;
unsigned u;
void f(void) {
    a + b * c;
    a = b = c;
    a - b - c;
    *p++;
    (char)a + d;
    a < b == c > a;
    a && b || !c;
    a ? b : c ? a : b;
    -a + ~b;
    a <<= 2, u >> 1;
    pp[1][2] = &a == p;
    sizeof(int *) + sizeof a;
    ++a;
    --*p;
    u = -1;
    d *= a;
}
