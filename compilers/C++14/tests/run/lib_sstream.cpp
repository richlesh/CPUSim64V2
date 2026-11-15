#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <iomanip>

struct Point { int x, y; };
std::ostream &operator<<(std::ostream &os, const Point &p) { return os << "(" << p.x << "," << p.y << ")"; }

int main()
{
    std::ostringstream os;
    os << "x=" << 42 << " pi=" << std::fixed << std::setprecision(2) << 3.14159 << ' ' << Point{1, 2};
    std::string s = os.str();
    std::cout << s << " [" << s.size() << "]" << std::endl;
    std::istringstream is("10 20 30 hello 2.5");
    int a, b, c;
    std::string w;
    double d;
    is >> a >> b >> c >> w >> d;
    std::cout << a + b + c << " " << w << " " << d << std::endl;
    int extra;
    is >> extra;
    std::cout << (is.fail() ? "fail" : "ok") << std::endl;
    std::istringstream lines("first line\nsecond line\nthird");
    std::string line;
    std::vector<std::string> all;
    while (std::getline(lines, line)) all.push_back(line);
    std::cout << all.size() << " " << all[1] << " " << all[2] << std::endl;
    std::map<std::string, std::vector<int>> index;
    std::istringstream text("a b a c b a");
    std::string word;
    int pos = 0;
    while (text >> word) index[word].push_back(pos++);
    for (auto &kv : index) {
        std::cout << kv.first << ":";
        for (int p : kv.second) std::cout << " " << p;
        std::cout << "\n";
    }
    std::stringstream ss;
    ss << 7 << " " << 8;
    int x, y;
    ss >> x >> y;
    std::cout << x * y << std::endl;
    std::ostringstream hex;
    hex << std::hex << 255 << " " << std::setw(5) << std::setfill('0') << 42;
    std::cout << hex.str() << std::endl;
    return 0;
}
