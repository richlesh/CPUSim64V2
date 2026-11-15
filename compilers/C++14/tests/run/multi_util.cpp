#include <stdexcept>
#include <string>

int util_calls = 0;

int checked_div(int a, int b)
{
    ++util_calls;
    if (b == 0) throw std::runtime_error("division by zero");
    return a / b;
}
