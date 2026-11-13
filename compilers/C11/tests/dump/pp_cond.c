#define A 2
#if A == 1
one
#elif A == 2
two
#  if 0
nested_never
#  else
nested_else
#  endif
#elif A == 3
three
#else
other
#endif
#if !defined A || (A * 3 - 1) % 4 != 1 || (1 ? 0 : 1)
bad
#else
good
#endif
#undef A
#ifndef A
undef_ok
#endif
#if 'a' == 97 && 0x10 == 16 && 010 == 8 && (-1 >> 1) == -1 && (1 << 4) == 16
chars_ok
#endif
