#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <type_traits>

namespace app::util {
    inline int counter = 10;
    int twice(int x) { return 2 * x; }
}

struct Config {
    static inline int version = 3;
    int w, h;
};

template<class T> int describe(const T &v)
{
    if constexpr (std::is_integral<T>::value) return (int)v * 10;
    else if constexpr (std::is_same<T, std::string>::value) return (int)v.size();
    else return -1;
}

[[nodiscard]] int compute() { return 42; }

std::pair<int, std::string> lookup() { return {7, "seven"}; }

int main()
{
    printf("%d %d\n", app::util::counter, app::util::twice(4));
    printf("%d\n", Config::version);
    Config c{640, 480};
    auto [w, h] = c;
    printf("%d %d\n", w, h);
    auto &[rw, rh] = c;
    rw = 800;
    printf("%d\n", c.w);
    int arr[3] = {1, 2, 3};
    auto [a0, a1, a2] = arr;
    printf("%d\n", a0 + a1 + a2);
    auto [num, name] = lookup();
    printf("%d %s\n", num, name.c_str());
    std::map<std::string, int> m = {{"x", 1}, {"y", 2}};
    for (const auto &[k, v] : m) printf("%s=%d ", k.c_str(), v);
    printf("\n");
    printf("%d %d %d\n", describe(5), describe(std::string("abcd")), describe(2.5));
    if (int x = compute(); x > 40) printf("big %d\n", x);
    else printf("small %d\n", x);
    switch (int k = 2; k * 2) {
    case 4: printf("four\n"); break;
    default: printf("other\n");
    }
    [[maybe_unused]] int unused = 0;
    return 0;
}
