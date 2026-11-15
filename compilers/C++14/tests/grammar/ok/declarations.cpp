// simple declarations and declarators
int a;
int b = 1, c(2), d{3};
const int k = 10;
static unsigned long long big = 1ULL << 40;
extern int ext;
char *p, **pp, &r = *p;
int arr[10], mat[3][4];
int *ap[5];
int (*pa)[5];
int (*fp)(int, char);
void (*handlers[4])(int);
typedef unsigned int uint;
typedef int (*cmp_t)(const void *, const void *);
typedef struct Node { int v; struct Node *next; } Node;
uint u = 5;
Node *head = nullptr;
auto inferred = 3.14;
decltype(inferred) other = 2.0;
bool flag = true;
wchar_t wc = L'x';
char16_t c16 = u'x';
char32_t c32 = U'x';
const char *str = "hello" " world";
const char *raw = R"(raw "string")";
long double ld = 1.5e3L;
float fl = 2.5f;
int hex = 0xFF, oct = 077, bin = 0b1010, sep = 1'000'000;
int f1(int);
int f2(int x, int y = 3, ...);
void f3() noexcept;
inline int f4(int x) { return x * x; }
constexpr int f5(int n) { return n <= 1 ? 1 : n * f5(n - 1); }
static const int sc = f5(4);
int f6(int (&arr)[3]);
int f7(int (*fn)(int));
int *f8();
int &f9(int &x);
auto f10(int x) -> int { return x; }
auto f11() -> decltype(f5(3));
void f12(const int *const p, volatile int *q);
int main() { return 0; }
