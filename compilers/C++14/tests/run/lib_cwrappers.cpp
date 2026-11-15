#include <cstdio>
#include <cmath>
#include <cassert>
#include <cstdint>
#include <ctime>
#include <cfloat>

int main()
{
    printf("%.4f %.4f %.4f\n", std::sqrt(2.0), std::hypot(3.0, 4.0), std::cbrt(27.0));
    printf("%g %g %g %g\n", std::round(2.5), std::trunc(-2.7), std::fmax(1.0, 3.0), std::log2(8.0));
    printf("%d %d\n", std::isnan(0.0 / 0.0) != 0, std::isinf(1.0 / 0.0) != 0);
    assert(1 + 1 == 2);
    std::int64_t big = INT32_MAX;
    printf("%ld\n", (long)(big + 1));
    std::time_t t = std::time(0);
    printf("%d\n", t > 1000000000);
    printf("%d\n", DBL_DIG);
    return 0;
}
