#include <iostream>
#include <string>

using namespace std;

int main()
{
    int a, b;
    cin >> a >> b;
    cout << "sum " << a + b << endl;
    double x;
    cin >> x;
    cout << "x " << x * 2 << endl;
    string w;
    cin >> w;
    cout << "word " << w << endl;
    char c;
    cin >> c;
    cout << "char " << c << endl;
    string line;
    getline(cin, line);
    getline(cin, line);
    cout << "line [" << line << "]" << endl;
    int n = 0, total = 0, v;
    while (cin >> v) { n++; total += v; }
    cout << n << " " << total << endl;
    cout << (cin.eof() ? "eof" : "not eof") << endl;
    return 0;
}
