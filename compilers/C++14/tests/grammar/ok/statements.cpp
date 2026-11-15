struct P { int x; };
P pa[3];
int g(int);
int body(int n, int *a) {
    int s = 0;
    if (n > 0) s = 1; else if (n < 0) s = -1; else s = 0;
    if (int t = g(n)) s += t;
    switch (n) {
    case 0: s++; break;
    case 1: case 2: { s += 2; break; }
    default: s--;
    }
    while (n > 0) { n--; continue; }
    do { n++; } while (n < 5);
    for (int i = 0; i < n; i++) s += i;
    for (;;) { break; }
    for (int *q = a; q != a + n; ++q) s += *q;
    for (auto x : a) s += x;
    for (const P &p : pa) s += p.x;
    goto done;
done:
    ;
    try { s = g(s); } catch (int e) { s = e; } catch (const P &p) { s = p.x; } catch (...) { throw; }
    { int inner = 3; s += inner; }
    return s;
}
int fn_try() try { return 1; } catch (...) { return 2; }
