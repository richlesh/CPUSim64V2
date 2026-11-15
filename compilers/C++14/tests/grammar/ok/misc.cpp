extern "C" { int c_func(int); }
extern "C" int c_func2(int);
namespace A { int x; namespace B { int y; } }
namespace AB = A::B;
using namespace A;
using A::x;
using IntPtr = int *;
template<class T> using Ptr = T *;
static_assert(sizeof(int) >= 4, "int too small");
namespace { int hidden; }
inline namespace V1 { int v; }
[[noreturn]] void die();
[[nodiscard]] int checked();
void takes(int a[3], int *const, const int *);
int main() { return A::B::y + AB::y + x; }
