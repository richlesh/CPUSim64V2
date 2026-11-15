#include <cstdio>

enum Color { Red, Green = 5, Blue };
enum class Mode : unsigned char { Off, On = 3, Auto };
enum { Anon1 = 10, Anon2 };

const char *name(Color c)
{
    switch (c) {
    case Red: return "red";
    case Green: return "green";
    case Blue: return "blue";
    }
    return "?";
}

int main()
{
    Color c = Green;
    printf("%s %d %d\n", name(c), c, Blue);
    c = Blue;
    printf("%s\n", name(c));
    Mode m = Mode::On;
    printf("%d %d\n", (int)m, static_cast<int>(Mode::Auto));
    if (m == Mode::On) printf("on\n");
    printf("%d %d\n", Anon1, Anon2);
    int arr[Blue];
    arr[0] = Red;
    printf("%d\n", arr[0] + Green);
    Color d = static_cast<Color>(0);
    printf("%s\n", name(d));
    printf("%d\n", Color::Green);
    switch (m) {
    case Mode::Off: printf("off\n"); break;
    case Mode::On: printf("on again\n"); break;
    default: break;
    }
    return 0;
}
