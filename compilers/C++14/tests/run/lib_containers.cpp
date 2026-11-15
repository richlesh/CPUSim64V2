#include <cstdio>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <queue>
#include <string>
#include <vector>
#include <algorithm>

int main()
{
    std::list<int> l = {3, 1, 4, 1, 5};
    l.push_front(9);
    l.push_back(2);
    for (int x : l) printf("%d ", x);
    printf("(%d)\n", (int)l.size());
    l.sort();
    for (int x : l) printf("%d ", x);
    printf("\n");
    l.unique();
    l.reverse();
    for (auto it = l.begin(); it != l.end(); ++it) printf("%d ", *it);
    printf("\n");
    l.remove(4);
    l.remove_if([](int x) { return x > 5; });
    for (int x : l) printf("%d ", x);
    printf("%d %d\n", l.front(), l.back());
    auto pos = std::find(l.begin(), l.end(), 3);
    l.insert(pos, 42);
    l.erase(l.begin());
    for (int x : l) printf("%d ", x);
    printf("\n");
    std::list<std::string> ls;
    ls.emplace_back("b");
    ls.emplace_front("a");
    std::list<std::string> ls2 = ls;
    ls2.splice(ls2.end(), ls);
    printf("%d %d %s\n", (int)ls.size(), (int)ls2.size(), ls2.back().c_str());

    std::unordered_map<std::string, int> counts;
    const char *words[] = {"the", "cat", "the", "dog", "the", "cat"};
    for (const char *w : words) counts[w]++;
    std::vector<std::string> keys;
    for (auto &kv : counts) keys.push_back(kv.first);
    std::sort(keys.begin(), keys.end());
    for (auto &k : keys) printf("%s=%d ", k.c_str(), counts[k]);
    printf("(%d)\n", (int)counts.size());
    printf("%d %d\n", (int)counts.count("cat"), (int)counts.count("cow"));
    counts.erase("cat");
    printf("%d\n", (int)counts.size());
    std::unordered_map<int, int> sq;
    for (int i = 0; i < 1000; i++) sq[i] = i * i;
    long sum = 0;
    for (auto &kv : sq) sum += kv.second;
    printf("%ld %d\n", sum, sq[31]);
    std::unordered_set<int> us = {1, 2, 3, 2, 1};
    printf("%d %d\n", (int)us.size(), (int)us.count(2));
    us.insert(10);
    us.erase(1);
    int s = 0;
    for (int x : us) s += x;
    printf("%d\n", s);

    std::stack<int> st;
    for (int i = 0; i < 4; i++) st.push(i);
    while (!st.empty()) { printf("%d ", st.top()); st.pop(); }
    printf("\n");
    std::queue<std::string> q;
    q.push("x");
    q.push("y");
    printf("%s %s %d\n", q.front().c_str(), q.back().c_str(), (int)q.size());
    q.pop();
    printf("%s\n", q.front().c_str());
    std::priority_queue<int> pq;
    for (int x : {5, 1, 8, 3, 9, 2}) pq.push(x);
    while (!pq.empty()) { printf("%d ", pq.top()); pq.pop(); }
    printf("\n");
    std::priority_queue<int, std::vector<int>, std::greater<int>> minq;
    for (int x : {5, 1, 8}) minq.push(x);
    printf("%d\n", minq.top());
    return 0;
}
