// Counts the words of the input and prints them by frequency.
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <cctype>

int main()
{
    std::map<std::string, int> counts;
    std::string word;
    while (std::cin >> word) {
        std::string w;
        for (char c : word)
            if (std::isalpha(c)) w += (char)std::tolower(c);
        if (!w.empty()) counts[w]++;
    }
    std::vector<std::pair<std::string, int>> v(counts.begin(), counts.end());
    std::sort(v.begin(), v.end(), [](const std::pair<std::string, int> &a, const std::pair<std::string, int> &b) {
        return a.second != b.second ? a.second > b.second : a.first < b.first;
    });
    for (const auto &p : v) std::cout << p.second << " " << p.first << "\n";
    return 0;
}
