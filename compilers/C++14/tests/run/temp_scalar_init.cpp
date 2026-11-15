// class temporaries in the initializer of a scalar variable are destroyed once
#include <cstdio>
#include <cstring>
#include <string>

static int live = 0;

struct Tracked {
    std::string s;
    explicit Tracked(const char *p) : s(p) { live++; }
    Tracked(const Tracked &o) : s(o.s) { live++; }
    ~Tracked() { live--; }
    long size() const { return (long)s.size(); }
};

static long len(const std::string &s) { return (long)s.size(); }
static Tracked make(const char *p) { return Tracked(p); }

static long g = len(std::string("global") + "!");      // a dynamic initializer of a global

int main()
{
    std::string a = "abc";
    long r1 = (a + ".x").size();
    const char *p = (a + ".pgm").c_str();               // dangling, but only compared with null
    auto r2 = len(a + "12");
    long r3{len(a + "1234")};
    long r4(make("hello").size());
    long r5 = r1 > 0 ? make("xy").size() : make("xyz").size();
    static long r6 = len(a + a);
    long r7 = (a + "1" + "2").size();
    std::printf("%ld %d %ld %ld %ld %ld %ld %ld %ld\n", r1, p != nullptr, r2, r3, r4, r5, r6, r7, g);
    std::printf("live %d\n", live);
    return 0;
}
