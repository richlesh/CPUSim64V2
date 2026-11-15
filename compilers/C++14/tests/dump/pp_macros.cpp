#define OBJ 42
#define EMPTY
#define SQ(x) ((x) * (x))
#define ADD(a, b) ((a) + (b))
#define STR(x) #x
#define XSTR(x) STR(x)
#define CAT(a, b) a ## b
#define XCAT(a, b) CAT(a, b)
#define VA(...) f(__VA_ARGS__)
#define VB(fmt, ...) printf(fmt, __VA_ARGS__)
#define VC(fmt, args...) printf(fmt, args)
#define LOG(fmt, ...) printf(fmt, ## __VA_ARGS__)
#define NEST(x) SQ(ADD(x, OBJ))
#define SELF SELF + 1
#define LOOP_A LOOP_B
#define LOOP_B LOOP_A
#define LPAREN (
#define APPLY(m, a) m a
#define FIRST(a, ...) a
#define REST(a, ...) __VA_ARGS__
int a = OBJ EMPTY + SQ(3) + ADD(1, SQ(2));
const char *s = STR(  hello   "w\"orld"  'c'  ) XSTR(OBJ) XSTR(ADD(1,2));
int CAT(var, 12) = XCAT(OBJ, 1);
VA() VA(1) VA(1, 2, 3)
VB("%d", 1, 2)
VC("%d", 1, 2)
LOG("hi") LOG("hi %d", 1)
NEST(5)
SELF LOOP_A
APPLY(SQ, (7))
SQ
(8) SQ EMPTY (9)
FIRST(1, 2, 3) REST(1, 2, 3) REST(1)
STR(@) STR(a + b) STR("a\n") STR('\0')
#define EMPTYARG(x) [x]
EMPTYARG() EMPTYARG( )
