#include <cstdio>

namespace util {
    int twice(int x) { return 2 * x; }
    int value = 5;
    namespace deep {
        int thrice(int x) { return 3 * x; }
        const int K = 4;
    }
}

namespace util {
    int plus(int a, int b) { return a + b; }
}

namespace {
    int hidden = 8;
    int hide(int x) { return x + hidden; }
}

namespace u = util;
namespace d = util::deep;

int twice(int x) { return x * 22; }

namespace shapes {
    int area(int w, int h) { return w * h; }
    double area(double r) { return 3.0 * r * r; }
}
using shapes::area;

int main()
{
    printf("%d %d\n", util::twice(4), twice(1));
    printf("%d %d\n", util::deep::thrice(3), d::thrice(2));
    printf("%d %d\n", u::value, util::deep::K);
    util::value = 9;
    printf("%d %d\n", u::value, u::plus(1, 2));
    printf("%d\n", hide(2));
    printf("%d %.1f\n", area(3, 4), area(2.0));
    {
        using namespace util;
        printf("%d\n", value + plus(1, 1));
    }
    using namespace util::deep;
    printf("%d\n", thrice(K));
    return 0;
}
