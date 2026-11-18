/*
 * nodes.inc - the Rust AST: node kinds, flag names, operators.
 *
 * Included several times with one of N, F or OP defined (see ast.h / ast.c):
 *   N(KIND)       a node kind, enum N_KIND.  The comment says what the fields
 *                 mean.  Fields not mentioned are 0 / empty.
 *   F(NAME)       a flag bit F_NAME (value in ast.h); the dump prints the lower
 *                 case name.  Bits are shared between kinds that mean the same.
 *   OP(NAME,"s")  an operator OP_NAME with its spelling.
 *
 * Every node also has: kind, line, col, file, attrs (outer and inner N_ATTR
 * nodes, in source order), ty and sym (filled in by later stages).  Items,
 * fields and variants have vis (an N_VIS node, or 0 when private/inherited).
 *
 * Notation: a b c d = the fixed children, list = the NodeList, name / name2 =
 * interned names, str = byte string, ival, flags, op, toks.  "T" = a type
 * node (N_T_xxx), "P" = a pattern node (N_P_xxx), "E" = an expression node,
 * "item" = an N_ITEM_xxx node.  A lifetime name includes the apostrophe:
 * 'a, 'static, '_ .
 *
 * Shorthand names and absent parts: an absent type is 0 (never N_T_INFER unless
 * the source wrote `_`).  The unit type / pattern / value `()` is an
 * N_T_TUPLE / N_P_TUPLE / N_E_TUPLE with an empty list.  Parentheses around a
 * type or pattern leave no node; parentheses around an expression give N_E_PAREN.
 */

#ifdef N

/* ===================================================================== crate */
N(CRATE)         /* list = items; attrs = the inner attributes (#![...]); str = file name if known */

/* ============================================================== attributes */
N(ATTR)          /* #[path ...] / #![...] / a doc comment.
                  * name = the path joined with "::" (derive, rustfmt::skip, doc);
                  * flags: INNER (#![..]), DOC (a doc comment: name "doc", str = the text,
                  * flags EQ too), EQ (`#[name = lit]`: a = literal E; for DOC there is no a),
                  * DELIM_PAREN / DELIM_BRACKET / DELIM_BRACE when written `name(...)`;
                  * toks / ntoks = the tokens between the delimiters (not for EQ). */
N(VIS)           /* visibility: flags VIS_CRATE (pub(crate)), VIS_SUPER, VIS_SELF, VIS_IN
                  * (pub(in a = N_PATH)); none of them = plain `pub`. Private = no N_VIS (0). */

/* ================================================================== paths */
N(PATH)          /* list = N_PATH_SEG.  flags: GLOBAL (leading ::), QUALIFIED (`<a as b>::..`:
                  * a = T self type, b = N_PATH of the trait or 0 for `<T>::x`; list = the
                  * segments after the `>::`).  Used for paths in types, expressions, patterns,
                  * bounds, use trees, visibility and macro names. */
N(PATH_SEG)      /* name = identifier, or self, Self, super, crate, $crate.
                  * list = generic arguments, in source order, each one of: N_LIFETIME, a type T,
                  * a const argument E (literal, `-literal`, block, or a bare path: a path
                  * argument is a T_PATH; the resolver decides), N_ASSOC_BIND.
                  * flags: ANGLE (`<...>` written, even if empty), TURBOFISH (`::<`),
                  * PARENS (Fn(A, B) -> C sugar: list = the input T's, a = output T or 0). */
N(LIFETIME)      /* name = 'a / 'static / '_ */
N(ASSOC_BIND)    /* `Item = T` (a = T) or `Item: Bound` (b = N_BOUNDS); name = the associated name */

/* =============================================================== generics */
N(GENERICS)      /* <...> parameter list: list = N_GEN_LIFETIME / N_GEN_TYPE / N_GEN_CONST.
                  * Also `for<'a, 'b>` (then only lifetimes). */
N(GEN_LIFETIME)  /* name = 'a; b = N_BOUNDS of N_BOUND_LIFETIME (`'a: 'b + 'c`) or 0 */
N(GEN_TYPE)      /* name; b = N_BOUNDS or 0; a = default T or 0 */
N(GEN_CONST)     /* const N: T = e: name; a = T; b = default E or 0 */
N(WHERE)         /* where clause: list = N_WHERE_TYPE / N_WHERE_LIFETIME */
N(WHERE_TYPE)    /* `for<'a> T: B + C`: a = T; b = N_BOUNDS; c = N_GENERICS (the for<...>) or 0 */
N(WHERE_LIFETIME) /* `'a: 'b + 'c`: name = 'a; b = N_BOUNDS of lifetimes */
N(BOUNDS)        /* list = N_BOUND_TRAIT / N_BOUND_LIFETIME, in order (`A + 'a + ?Sized`) */
N(BOUND_TRAIT)   /* a = N_PATH of the trait; b = N_GENERICS (for<'a>) or 0; flags MAYBE (?Sized), PAREN (written in parentheses) */
N(BOUND_LIFETIME) /* name = 'a */

/* ==================================================================== items */
/* Every item has attrs and vis (0 if private).  Items appear in N_CRATE, N_ITEM_MOD,
 * N_BLOCK (as statements), traits, impls and extern blocks. */
N(ITEM_FN)       /* name; a = N_GENERICS or 0; list = N_PARAM (a leading N_SELF_PARAM for methods);
                  * b = return T or 0 (unit); c = body N_BLOCK, 0 for `fn f();` (trait / extern);
                  * d = N_WHERE or 0; flags CONST, ASYNC, UNSAFE, EXTERN (str = ABI string or
                  * 0), VARIADIC (C `...` last parameter). */
N(PARAM)         /* a = P (0 for an unnamed parameter such as fn(i32) or a trait method `fn f(i32)`); b = T (0 for a closure
                  * parameter without type); attrs allowed. */
N(SELF_PARAM)    /* self, &self, &mut self, &'a self, mut self, self: T.
                  * flags REF (&), MUT (mut self, or &mut self), TYPED (self: T, b = T);
                  * a = N_LIFETIME for &'a self. */
N(ITEM_STRUCT)   /* name; a = N_GENERICS; d = N_WHERE; list = N_FIELD;
                  * flags NAMED (`{...}`, even if empty) or TUPLE (`(...)`); neither = unit struct */
N(FIELD)         /* a named or tuple field: name (0 for tuple fields: the position is the index);
                  * a = T; vis; attrs */
N(ITEM_ENUM)     /* name; a = N_GENERICS; d = N_WHERE; list = N_VARIANT */
N(VARIANT)       /* name; list = N_FIELD; flags NAMED / TUPLE / neither (unit);
                  * a = discriminant E or 0; attrs; vis */
N(ITEM_UNION)    /* like N_ITEM_STRUCT with NAMED; list = N_FIELD */
N(ITEM_TRAIT)    /* name; a = N_GENERICS; c = N_BOUNDS of the supertraits or 0; d = N_WHERE;
                  * list = associated items (N_ITEM_FN, N_ITEM_CONST, N_ITEM_TYPE, N_MACRO_CALL);
                  * flags UNSAFE, AUTO; inner attrs are in attrs with flag INNER */
N(ITEM_IMPL)     /* a = N_GENERICS; b = self T; c = N_PATH of the trait or 0 (inherent impl);
                  * d = N_WHERE; list = associated items; flags UNSAFE, NEG (`impl !Trait for T`) */
N(ITEM_MOD)      /* name; list = items; flags INLINE (has a `{ }` body; otherwise `mod x;`), UNSAFE */
N(ITEM_USE)      /* a = N_USE_TREE */
N(USE_TREE)      /* a = N_PATH prefix (simple path, may be 0 for `use {a, b}` or `use *`).
                  * flags GLOB (path::*  or  *), GROUP (path::{list}: list = N_USE_TREE);
                  * neither = a leaf `path` or `path as name2` (name2 = alias, "_" for `as _`). A leaf
                  * `use a::b::{self}` has the segment `self` in its path. */
N(ITEM_CONST)    /* name ("_" allowed); a = T; b = E or 0 (in traits); also an associated const */
N(ITEM_STATIC)   /* name; a = T; b = E or 0 (extern blocks); flags MUT */
N(ITEM_TYPE)     /* type alias or associated type: name; a = N_GENERICS; b = T or 0; c = N_BOUNDS (`type X: Bound;`) or 0; d = N_WHERE */
N(ITEM_EXTERN_CRATE) /* name = crate ("self" ok); name2 = alias ("_" ok) or 0 */
N(ITEM_EXTERN_BLOCK) /* str = ABI string or 0; list = N_ITEM_FN (no body), N_ITEM_STATIC, N_ITEM_TYPE,
                  * N_MACRO_CALL; flags UNSAFE */
N(ITEM_MACRO_RULES) /* macro_rules! name { ... }: name; list = N_MACRO_RULE; flags DELIM_xxx of the definition */
N(MACRO_RULE)    /* a = N_MACRO_TT matcher; b = N_MACRO_TT transcriber */
N(MACRO_TT)      /* a token tree body: toks / ntoks = the tokens between the delimiters,
                  * flags DELIM_PAREN / DELIM_BRACKET / DELIM_BRACE */
N(MACRO_CALL)    /* path!(...): a = N_PATH; toks / ntoks = the tokens between the delimiters;
                  * flags DELIM_xxx, SEMI (followed by `;`). The one node used for a macro in
                  * any position: item, statement, expression, type, pattern, associated
                  * item.  In a block's list it is a macro statement; as a block tail it is in a. */

/* ==================================================================== types */
N(T_PATH)        /* a = N_PATH (primitives, Self, generics and qualified paths all land here) */
N(T_REF)         /* &'a mut T: a = T; b = N_LIFETIME or 0; flags MUT */
N(T_PTR)         /* *const T / *mut T: a = T; flags MUT */
N(T_ARRAY)       /* [T; n]: a = T; b = length E */
N(T_SLICE)       /* [T]: a = T */
N(T_TUPLE)       /* (A, B,) : list; `()` is the empty list */
N(T_NEVER)       /* ! */
N(T_FN)          /* fn(A, B) -> R: list = N_PARAM (a is usually 0, or a P_IDENT / P_WILD for a written name); b = return T or 0;
                  * c = N_GENERICS (for<'a>) or 0; flags UNSAFE, EXTERN (str = ABI or 0), VARIADIC */
N(T_DYN)         /* a = N_BOUNDS; flags DYN (written with `dyn`; without it is a bare trait object) */
N(T_IMPL)        /* impl A + B: a = N_BOUNDS */
N(T_INFER)       /* _ */

/* ================================================================== patterns */
N(P_WILD)        /* _ */
N(P_REST)        /* .. (in tuple, slice and tuple-struct patterns) */
N(P_IDENT)       /* [ref] [mut] name [@ sub]: name; a = sub pattern or 0; flags REF, MUT */
N(P_LIT)         /* a = literal E (N_E_LIT_xxx); flags NEG for -1 / -1.5 */
N(P_RANGE)       /* a..=b, a..b, a.., ..=b : a, b = N_P_LIT or N_P_PATH or 0; flags INCLUSIVE (`..=` and `...`) */
N(P_REF)         /* &p, &mut p ( && p is two nested N_P_REF): a = P; flags MUT */
N(P_TUPLE)       /* list (may contain N_P_REST); `(p)` is not a tuple, `(p,)` is */
N(P_SLICE)       /* [a, b, rest @ ..]: list (may contain N_P_REST or N_P_IDENT with a = N_P_REST) */
N(P_PATH)        /* a = N_PATH: a unit struct / variant / constant / (qualified) path.
                  * A lone identifier is N_P_IDENT; the resolver turns it into a path if it names a const / unit variant. */
N(P_TUPLE_STRUCT) /* a = N_PATH; list = patterns (may contain N_P_REST) */
N(P_STRUCT)      /* a = N_PATH; list = N_P_FIELD; flags REST (`..` written) */
N(P_FIELD)       /* name (a tuple index is the name "0"); a = P; flags SHORTHAND (`x`, `ref x`, `mut x`: a is then an N_P_IDENT); attrs */
N(P_OR)          /* list = alternatives (>= 2) */
/* a macro in pattern position is an N_MACRO_CALL */

/* ================================================================ expressions */
N(E_LIT_INT)     /* ival = the value as a 64-bit pattern (u64 beyond i64 wraps); name = suffix (u8, i32, usize...) or 0;
                  * str = the spelling (without suffix) */
N(E_LIT_FLOAT)   /* fval = the value; name = suffix (f32, f64) or 0; str = the spelling (without suffix) */
N(E_LIT_STR)     /* str / slen = the UTF-8 bytes of the value (escapes decoded); flags RAW */
N(E_LIT_BYTESTR) /* b"..": str / slen = the bytes; flags RAW */
N(E_LIT_CHAR)    /* ival = the Unicode scalar value */
N(E_LIT_BYTE)    /* b'x': ival = the byte */
N(E_LIT_BOOL)    /* ival = 0 / 1 */
N(E_PATH)        /* a = N_PATH (variables, functions, consts, unit structs, enum variants, qualified paths) */
N(E_UNDERSCORE)  /* `_` as an expression (destructuring assignment) */
N(E_PAREN)       /* (a) */
N(E_UNARY)       /* op = OP_NEG (-a) or OP_NOT (!a); a */
N(E_DEREF)       /* *a */
N(E_REF)         /* &a, &mut a (&&a is two nested N_E_REF): a; flags MUT */
N(E_TRY)         /* a? */
N(E_BINARY)      /* a op b: op = OP_ADD SUB MUL DIV REM BITAND BITOR BITXOR SHL SHR EQ NE LT LE GT GE LAND LOR */
N(E_ASSIGN)      /* a = b */
N(E_ASSIGN_OP)   /* a op= b: op = OP_ADD SUB MUL DIV REM BITAND BITOR BITXOR SHL SHR */
N(E_CAST)        /* a as T: a = E; b = T */
N(E_CALL)        /* a(args): a = callee E; list = args */
N(E_METHOD_CALL) /* a.name::<..>(args): a = receiver; name = method; list = args;
                  * b = N_PATH_SEG (same name, with the turbofish arguments) only if a turbofish was written, else 0 */
N(E_FIELD)       /* a.name: a = E; name */
N(E_TUPLE_FIELD) /* a.0: a = E; ival = the index */
N(E_INDEX)       /* a[b] */
N(E_RANGE)       /* a..b, a..=b, a.., ..b, ..=b, ..: a, b = E or 0; flags INCLUSIVE */
N(E_AWAIT)       /* a.await (parse only) */
N(BLOCK)         /* { stmts tail }: list = statements; a = tail E (no trailing `;`) or 0;
                  * name = label ('a: { }) or 0; flags UNSAFE (`unsafe { }`), ASYNC (`async { }` /
                  * `async move { }` with MOVE), CONST (`const { }`); inner attrs in attrs with flag INNER.
                  * A block is also the body of fn / if / loops. */
N(E_IF)          /* if a { b } else c: a = condition E (an N_E_LET for `if let`; `&&` chains of them are
                  * N_E_BINARY OP_LAND); b = N_BLOCK; c = N_BLOCK or N_E_IF or 0 */
N(E_LET)         /* `let p = e` inside an if / while condition: a = P; b = E (scrutinee) */
N(E_MATCH)       /* match a { arms }: a = scrutinee; list = N_ARM; inner attrs in attrs with flag INNER */
N(ARM)           /* a = P (an N_P_OR for `A | B`); b = guard E or 0 (`if let` guard is N_E_LET); c = body E; attrs */
N(E_WHILE)       /* a = condition E (N_E_LET for while let); b = N_BLOCK; name = label or 0 */
N(E_LOOP)        /* a = N_BLOCK; name = label or 0 */
N(E_FOR)         /* for a in b { c }: a = P; b = iterator E; c = N_BLOCK; name = label or 0 */
N(E_BREAK)       /* name = label or 0; a = value E or 0 */
N(E_CONTINUE)    /* name = label or 0 */
N(E_RETURN)      /* a = value E or 0 */
N(E_CLOSURE)     /* list = N_PARAM (b = T or 0); a = return T or 0 (then the body is a block); b = body E;
                  * flags MOVE, ASYNC */
N(E_STRUCT)      /* a = N_PATH; list = N_FIELD_INIT; b = base E (`..base`) or 0 */
N(FIELD_INIT)    /* name (a tuple index is the name "0"); a = E; flags SHORTHAND (`x` alone: a = N_E_PATH x); attrs */
N(E_TUPLE)       /* list; `()` empty; `(a,)` one element */
N(E_ARRAY)       /* [a, b, c]: list */
N(E_REPEAT)      /* [a; b]: a = element E; b = count E */
/* a macro in expression position is an N_MACRO_CALL */

/* ================================================================== statements */
/* A block's list holds, in order: N_STMT_LET, N_STMT_EXPR, N_STMT_EMPTY, items (N_ITEM_xxx),
 * and N_MACRO_CALL (macro statement; flag SEMI if `;` followed). */
N(STMT_LET)      /* let a: b = c else d; : a = P; b = T or 0; c = init E or 0; d = else N_BLOCK or 0; attrs */
N(STMT_EXPR)     /* a = E; flags SEMI (ended with `;`).  An expression statement that is block-like
                  * (if, match, loops, blocks, unsafe) may omit the `;` and is not the tail. */
N(STMT_EMPTY)    /* a lone `;` */

#endif /* N */

/* ===================================================================== flags */
#ifdef F
F(MUT)           /* mut: ref / ptr types, borrows, ident patterns, static, self */
F(REF)           /* `ref` in an ident pattern; `&` in a self parameter */
F(CONST)
F(ASYNC)
F(UNSAFE)
F(EXTERN)
F(VARIADIC)
F(MOVE)
F(GLOBAL)
F(QUALIFIED)
F(TURBOFISH)
F(PARENS)
F(ANGLE)
F(NAMED)
F(TUPLE)
F(NEG)
F(INCLUSIVE)
F(MAYBE)
F(REST)
F(SHORTHAND)
F(TYPED)
F(GLOB)
F(GROUP)
F(INLINE)
F(AUTO)
F(SEMI)
F(DYN)
F(INNER)
F(DOC)
F(EQ)
F(RAW)
F(PAREN)
F(DELIM_PAREN)
F(DELIM_BRACKET)
F(DELIM_BRACE)
F(VIS_CRATE)
F(VIS_SUPER)
F(VIS_SELF)
F(VIS_IN)
#endif /* F */

/* =================================================================== operators */
#ifdef OP
OP(NEG, "-")     /* unary */
OP(NOT, "!")     /* unary */
OP(ADD, "+")
OP(SUB, "-")
OP(MUL, "*")
OP(DIV, "/")
OP(REM, "%")
OP(BITAND, "&")
OP(BITOR, "|")
OP(BITXOR, "^")
OP(SHL, "<<")
OP(SHR, ">>")
OP(EQ, "==")
OP(NE, "!=")
OP(LT, "<")
OP(LE, "<=")
OP(GT, ">")
OP(GE, ">=")
OP(LAND, "&&")
OP(LOR, "||")
#endif /* OP */
