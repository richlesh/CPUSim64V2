#include <cstdio>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>
#include <array>
#include <utility>

struct Item {
    int id;
    std::string name;
    Item(int i, const std::string &n) : id(i), name(n) {}
    bool operator<(const Item &o) const { return id < o.id; }
};

struct MoveOnly {
    int v;
    explicit MoveOnly(int x) : v(x) {}
    MoveOnly(MoveOnly &&o) : v(o.v) { o.v = -1; }
    MoveOnly &operator=(MoveOnly &&o) { v = o.v; o.v = -1; return *this; }
    MoveOnly(const MoveOnly &) = delete;
};

template<class C> void show(const char *name, const C &c)
{
    printf("%s:", name);
    for (typename C::const_iterator i = c.begin(); i != c.end(); ++i) printf(" %d", (int)*i);
    printf(" (size %d)\n", (int)c.size());
}

int main()
{
    std::vector<int> v;
    for (int i = 0; i < 10; i++) v.push_back(i * 3 % 7);
    show("v", v);
    std::sort(v.begin(), v.end());
    show("sorted", v);
    std::reverse(v.begin(), v.end());
    show("reversed", v);
    v.erase(v.begin() + 2, v.begin() + 5);
    show("erased", v);
    v.insert(v.begin() + 1, 99);
    v.insert(v.end(), 3, 7);
    show("inserted", v);
    printf("%d %d\n", v.front(), v.back());
    std::vector<int> w = {5, 3, 8, 1};
    std::sort(w.begin(), w.end(), [](int a, int b) { return a > b; });
    show("w", w);
    std::vector<int> z(4, 2);
    show("z", z);
    z.assign(w.begin(), w.end());
    show("z2", z);
    printf("%d\n", std::accumulate(z.begin(), z.end(), 0));
    printf("%d\n", (int)(std::find(z.begin(), z.end(), 3) - z.begin()));
    printf("%d\n", (int)std::count_if(z.begin(), z.end(), [](int x) { return x > 2; }));
    printf("%d %d\n", *std::min_element(z.begin(), z.end()), *std::max_element(z.begin(), z.end()));
    std::vector<Item> items;
    items.push_back(Item(3, "c"));
    items.emplace_back(1, "a");
    items.emplace_back(2, "b");
    std::sort(items.begin(), items.end());
    for (size_t i = 0; i < items.size(); i++) printf("%d%s ", items[i].id, items[i].name.c_str());
    printf("\n");
    std::vector<MoveOnly> mo;
    mo.emplace_back(1);
    mo.emplace_back(2);
    mo.emplace_back(3);
    mo.push_back(MoveOnly(4));
    int sum = 0;
    for (size_t i = 0; i < mo.size(); i++) sum += mo[i].v;
    printf("%d\n", sum);
    std::vector<std::string> names = {"delta", "alpha", "charlie", "bravo"};
    std::sort(names.begin(), names.end());
    for (auto &n : names) printf("%s ", n.c_str());
    printf("\n");
    std::vector<std::vector<int>> grid(3, std::vector<int>(3, 0));
    grid[1][2] = 5;
    int total = 0;
    for (auto &row : grid) for (int x : row) total += x;
    printf("%d\n", total);
    std::vector<int> big;
    for (int i = 0; i < 200; i++) big.push_back((i * 37) % 101);
    std::sort(big.begin(), big.end());
    printf("%d %d %d\n", big[0], big[100], big[199]);
    std::stable_sort(items.begin(), items.end(), [](const Item &a, const Item &b) { return a.name < b.name; });
    printf("%s\n", items[0].name.c_str());
    std::array<int, 4> a = {4, 2, 3, 1};
    std::sort(a.begin(), a.end());
    printf("%d %d %d\n", a[0], a[3], (int)a.size());
    std::pair<int, std::string> pp(1, "x");
    std::vector<std::pair<int, std::string>> pairs;
    pairs.push_back(std::make_pair(2, std::string("two")));
    pairs.push_back(pp);
    std::sort(pairs.begin(), pairs.end());
    printf("%d %s\n", pairs[0].first, pairs[1].second.c_str());
    try {
        v.at(100);
    } catch (const std::out_of_range &e) {
        printf("out_of_range\n");
    }
    std::vector<int> e1, e2;
    printf("%d %d\n", (int)(e1 == e2), (int)(v < w));
    v.clear();
    printf("%d\n", (int)v.empty());
    return 0;
}
