/*
 * keywords.inc - the C++14 keywords (ISO C++14 [lex.key]), without the
 * alternative operator tokens (and, or, not, ... see alt_ops in lexer.c),
 * which the lexer turns into punctuators.  override and final are
 * identifiers with a special meaning; the parser recognizes them by name.
 */
KW(alignas) KW(alignof) KW(asm) KW(auto)
KW(bool) KW(break) KW(case) KW(catch)
KW(char) KW(char16_t) KW(char32_t) KW(class)
KW(const) KW(constexpr) KW(const_cast) KW(continue)
KW(decltype) KW(default) KW(delete) KW(do)
KW(double) KW(dynamic_cast) KW(else) KW(enum)
KW(explicit) KW(export) KW(extern) KW(false)
KW(float) KW(for) KW(friend) KW(goto)
KW(if) KW(inline) KW(int) KW(long)
KW(mutable) KW(namespace) KW(new) KW(noexcept)
KW(nullptr) KW(operator) KW(private) KW(protected)
KW(public) KW(register) KW(reinterpret_cast) KW(return)
KW(short) KW(signed) KW(sizeof) KW(static)
KW(static_assert) KW(static_cast) KW(struct) KW(switch)
KW(template) KW(this) KW(thread_local) KW(throw)
KW(true) KW(try) KW(typedef) KW(typeid)
KW(typename) KW(union) KW(unsigned) KW(using)
KW(virtual) KW(void) KW(volatile) KW(wchar_t)
KW(while)
