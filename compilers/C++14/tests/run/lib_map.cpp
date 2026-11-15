#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>

int main()
{
    std::map<std::string, int> m;
    m["banana"] = 3;
    m["apple"] = 1;
    m["cherry"] = 7;
    m["apple"] += 10;
    for (std::map<std::string, int>::iterator i = m.begin(); i != m.end(); ++i) printf("%s=%d ", i->first.c_str(), i->second);
    printf("\n");
    printf("%d %d %d\n", (int)m.size(), (int)m.count("banana"), (int)m.count("kiwi"));
    m.erase("banana");
    for (auto &kv : m) printf("%s=%d ", kv.first.c_str(), kv.second);
    printf("\n");
    auto r = m.insert(std::make_pair(std::string("apple"), 0));
    printf("%d %d\n", (int)r.second, r.first->second);
    std::map<int, std::vector<int>> groups;
    for (int i = 0; i < 20; i++) groups[i % 3].push_back(i);
    for (auto &g : groups) {
        printf("%d:", g.first);
        for (int x : g.second) printf(" %d", x);
        printf("\n");
    }
    std::map<int, int> big;
    for (int i = 0; i < 500; i++) big[(i * 7919) % 1000] = i;
    int prev = -1, ok = 1;
    for (auto &kv : big) { if (kv.first <= prev) ok = 0; prev = kv.first; }
    printf("%d %d\n", (int)big.size(), ok);
    for (int i = 0; i < 1000; i += 2) big.erase(i);
    prev = -1;
    for (auto &kv : big) { if (kv.first <= prev || kv.first % 2 == 0) ok = 0; prev = kv.first; }
    printf("%d %d\n", (int)big.size(), ok);
    auto lb = big.lower_bound(500);
    printf("%d\n", lb->first);
    std::map<int, int>::iterator last = big.end();
    --last;
    printf("%d\n", last->first);
    std::set<int> s = {5, 1, 4, 1, 5, 9, 2, 6};
    for (int x : s) printf("%d ", x);
    printf("\n");
    s.insert(3);
    s.erase(9);
    printf("%d %d %d\n", (int)s.size(), (int)s.count(3), (int)(s.find(9) == s.end()));
    for (auto it = s.rbegin(); it != s.rend(); ++it) printf("%d ", *it);
    printf("\n");
    std::multiset<int> ms = {3, 1, 3, 2, 3};
    printf("%d %d\n", (int)ms.size(), (int)ms.count(3));
    std::multimap<std::string, int> mm;
    mm.insert(std::make_pair(std::string("a"), 1));
    mm.insert(std::make_pair(std::string("a"), 2));
    mm.insert(std::make_pair(std::string("b"), 3));
    printf("%d %d\n", (int)mm.size(), (int)mm.count("a"));
    std::map<std::string, int> copy = m;
    copy["zzz"] = 1;
    printf("%d %d\n", (int)m.size(), (int)copy.size());
    std::map<int, std::string, std::greater<int>> desc = {{1, "one"}, {3, "three"}, {2, "two"}};
    for (auto &kv : desc) printf("%s ", kv.second.c_str());
    printf("\n");
    try {
        m.at("nothing");
    } catch (const std::out_of_range &) {
        printf("out_of_range\n");
    }
    return 0;
}
