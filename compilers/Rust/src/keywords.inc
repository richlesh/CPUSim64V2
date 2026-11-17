/*
 * keywords.inc - the Rust keywords that the lexer turns into TK_KW_xxx tokens:
 * the strict keywords, the 2018 additions (async await dyn), the reserved
 * keywords and `try` (reserved since 2018).  `union`, `macro_rules` and
 * `auto` are weak keywords and stay identifiers; the parser recognizes them
 * by name.  `r#name` is never a keyword.  Includers define KW(x).
 */
KW(as) KW(break) KW(const) KW(continue)
KW(crate) KW(else) KW(enum) KW(extern)
KW(false) KW(fn) KW(for) KW(if)
KW(impl) KW(in) KW(let) KW(loop)
KW(match) KW(mod) KW(move) KW(mut)
KW(pub) KW(ref) KW(return) KW(self)
KW(Self) KW(static) KW(struct) KW(super)
KW(trait) KW(true) KW(type) KW(unsafe)
KW(use) KW(where) KW(while)
KW(async) KW(await) KW(dyn)
KW(abstract) KW(become) KW(box) KW(do)
KW(final) KW(macro) KW(override) KW(priv)
KW(typeof) KW(unsized) KW(virtual) KW(yield)
KW(try)
