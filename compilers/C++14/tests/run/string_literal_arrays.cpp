// arrays of string literals: global and static arrays (constant initializers),
// and std::array and aggregates initialized with string literals (brace elision)
#include <array>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

static const char *const COLORS[] = {"red", "green", "blue"};
static const char *NAMES[4] = {"ann", "bob"};                 // the rest are null
const char *const GRID[2][2] = {{"a", "b"}, {"c", "d"}};
static const char WORD[] = "word";
static const char *TAIL = WORD + 1;
static const char *ONE = "one";

struct Entry {
    char code[4];
    const char *names[2];
};

static const Entry ENTRY = {"ab", "x", "y"};

std::array<const char *, 3> global_array = {"p", "q", "r"};

int main()
{
    std::cout << sizeof(COLORS) / sizeof(COLORS[0]) << ' ' << COLORS[0] << ' ' << COLORS[2] << '\n';
    std::cout << NAMES[1] << ' ' << (NAMES[2] == nullptr) << ' ' << (NAMES[3] == nullptr) << '\n';
    std::cout << GRID[0][1] << GRID[1][0] << ' ' << TAIL << ' ' << ONE << '\n';
    static const char *const local_static[] = {"s1", "s2"};
    const char *local[] = {"l1", "l2"};
    std::cout << local_static[1] << ' ' << local[0] << '\n';

    std::array<const char *, 2> a = {"a1", "a2"};
    std::array<const char *, 2> b{"b1", "b2"};
    std::array<const char *, 3> c = {"c1"};
    std::array<const char *, 2> d = {{"d1", "d2"}};
    std::array<std::string, 3> s = {"s", "t"};
    std::array<char, 4> chars = {"xyz"};
    std::cout << a[0] << a[1] << ' ' << b[1] << ' ' << c[0] << (c[1] == nullptr) << (c[2] == nullptr)
              << ' ' << d[1] << ' ' << s[0] + s[1] << '[' << s[2] << "] " << chars.data() << '\n';
    for (const char *p : global_array) std::cout << p;
    std::cout << '\n';
    std::cout << ENTRY.code << ' ' << std::strlen(ENTRY.code) << ' ' << ENTRY.names[0] << ENTRY.names[1] << '\n';
    Entry e = {"cd", "u"};
    std::cout << e.code << ' ' << e.names[0] << ' ' << (e.names[1] == nullptr) << '\n';
    return 0;
}
