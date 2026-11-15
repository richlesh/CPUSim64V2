#include <string>
#include <cstdio>

using std::string;

static void show(const char *label, const string &s)
{
    printf("%s=[%s] size=%d\n", label, s.c_str(), (int)s.size());
}

int main()
{
    string a = "hello", b("world");
    string c = a + ", " + b + '!';
    show("c", c);
    printf("%d %d\n", (int)c.find("wor"), c.find('z') == string::npos);
    string d = c.substr(7, 5);
    d += "?";
    show("d", d);
    printf("%d %d %d\n", d == "world?", d != c, a < b);
    string e;
    for (int i = 0; i < 5; i++) e += (char)('a' + i);
    show("e", e);
    e.insert(2, "XY");
    show("e", e);
    e.erase(0, 1);
    show("e", e);
    e.replace(0, 2, "__");
    show("e", e);
    printf("%d %d\n", (int)e.rfind('d'), (int)e.find_first_of("XY"));
    e.resize(3);
    show("e", e);
    e.push_back('!');
    e.pop_back();
    printf("%c %c %d\n", e[0], e.back(), e.empty());
    string f(3, 'z');
    show("f", f);
    string g = f;
    g[1] = 'a';
    show("f", f);
    show("g", g);
    g.swap(f);
    show("g", g);
    printf("%s %s %s\n", std::to_string(42).c_str(), std::to_string(-7L).c_str(), std::to_string(1.5).c_str());
    printf("%d %.1f\n", std::stoi("123") + 1, std::stod("2.5") * 2);
    string long_str;
    for (int i = 0; i < 100; i++) long_str += "ab";
    printf("%d\n", (int)long_str.size());
    int count = 0;
    for (const char *p = long_str.c_str(); *p; p++) if (*p == 'a') count++;
    printf("%d\n", count);
    string h = "abc";
    printf("%d %d %d\n", h.compare("abd"), h.compare("abc"), h.compare("abb"));
    return 0;
}
