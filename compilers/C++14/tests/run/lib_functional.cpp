#include <cstdio>
#include <functional>
#include <vector>
#include <algorithm>
#include <string>

int twice(int x) { return 2 * x; }

struct Adder {
    int k;
    int operator()(int x) const { return x + k; }
};

std::function<int(int)> compose(std::function<int(int)> f, std::function<int(int)> g)
{
    return [f, g](int x) { return f(g(x)); };
}

int main()
{
    std::function<int(int)> f = twice;
    printf("%d\n", f(21));
    f = Adder{5};
    printf("%d\n", f(1));
    int base = 100;
    f = [base](int x) { return x + base; };
    printf("%d\n", f(1));
    std::function<int(int)> g = compose(twice, Adder{1});
    printf("%d\n", g(4));
    std::function<void()> empty;
    printf("%d\n", (int)(bool)empty);
    try {
        empty();
    } catch (const std::bad_function_call &) {
        printf("bad_function_call\n");
    }
    std::function<std::string(const std::string &, int)> rep = [](const std::string &s, int n) {
        std::string r;
        for (int i = 0; i < n; i++) r += s;
        return r;
    };
    printf("%s\n", rep("ab", 3).c_str());
    std::vector<int> v = {5, 1, 4, 2, 3};
    std::sort(v.begin(), v.end(), std::greater<int>());
    for (int x : v) printf("%d ", x);
    printf("\n");
    printf("%d %d\n", std::plus<int>()(2, 3), std::multiplies<int>()(4, 5));
    std::vector<std::function<int()>> fs;
    for (int i = 0; i < 3; i++) fs.push_back([i] { return i * i; });
    for (auto &h : fs) printf("%d ", h());
    printf("\n");
    std::hash<std::string> hs;
    printf("%d\n", (int)(hs("abc") == hs(std::string("abc"))));
    printf("%d\n", (int)(std::hash<int>()(7) == 7));
    return 0;
}
