#define A 1
#define B 0
#if A && !B
one
#else
no
#endif
#if B
no
#elif A == 1
two
#else
no
#endif
#ifdef A
three
#endif
#ifndef A
no
#else
four
#endif
#if defined(A) && defined B || defined(NOPE)
five
#endif
#if 0
don't "break on this
#if 1
no
#else
no
#endif
#else
six
#endif
#if (2 + 3) * 4 == 20 && 7 / 2 == 3 && 7 % 4 == 3 && -1 < 0 && (1 << 4) == 16 && ~0 == -1
seven
#endif
#if 'a' == 97 && '\n' == 10 && '\x41' == 65 && '\101' == 65
eight
#endif
#if 0x10 == 16 && 010 == 8 && 0b101 == 5 && 1'000 == 1000 && 10UL == 10
nine
#endif
#if UNDEFINED_NAME == 0 && true && !false
ten
#endif
#if 1 ? 0 : 1
no
#elif 0 ? 1 : 2
eleven
#endif
#if A
#if B
no
#elif A
twelve
#endif
#endif
#if __cplusplus >= 201103L
thirteen
#endif
