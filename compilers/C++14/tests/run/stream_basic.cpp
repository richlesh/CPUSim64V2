#include <iostream>
#include <iomanip>
#include <string>

using namespace std;

int main()
{
    cout << "Hello, " << "World" << '!' << endl;
    int i = 42;
    long l = 1234567890123L;
    double d = 3.14159265;
    bool b = true;
    cout << i << " " << l << " " << d << " " << b << " " << 'x' << endl;
    cout << 1.0 << " " << 2.5e10 << " " << 1e-5 << " " << 100000.0 << " " << 1234567.0 << endl;
    cout << boolalpha << b << " " << false << noboolalpha << " " << true << endl;
    cout << hex << 255 << " " << oct << 8 << " " << dec << 10 << endl;
    cout << fixed << setprecision(2) << d << " " << 2.0 << endl;
    cout << scientific << setprecision(3) << 12345.678 << endl;
    cout << defaultfloat << setprecision(6);
    cout << "[" << setw(6) << i << "]" << "[" << setw(6) << left << i << "]" << "[" << right << setfill('*') << setw(6) << i << "]" << endl;
    cout << setfill(' ');
    string s = "text";
    cout << s << " " << s.size() << " " << s + "!" << endl;
    cout << "[" << setw(8) << s << "]" << endl;
    const char *p = "cstr";
    cout << p << endl;
    unsigned u = 4000000000U;
    cout << u << " " << (unsigned char)'A' << " " << (short)-5 << endl;
    cerr << "to stderr" << endl;
    cout << "a" << "b" << "c" << endl << "done" << endl;
    cout << 7 / 2.0 << " " << 10 % 3 << " " << (i > 10) << endl;
    return 0;
}
