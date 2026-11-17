# Lexer tests

Each `NAME.rs` is lexed with `rustc --tokens lex/NAME.rs` and the output (after
the banner, which ends with the `Arguments:` line) must equal `NAME.tokens`.
If `NAME.tokens-flags` exists, `rustc --tokens-flags lex/NAME.rs` must equal it
too.  `tests/run_tests.sh [filter]` runs them as `lex/NAME`, on the native
translator (`HOST=1`) and on the simulator.  These files are not Rust programs
that compile; they only exercise the lexer.  Lexical errors are tested in
`tests/errors/lex_*.rs`.

## Format

One line per token, in source order, ending with `EOF`:

    LINE:COL KIND text

`LINE` and `COL` are 1-based; `COL` counts codepoints from the start of the
line.  `text` by `KIND`:

| KIND | text |
|---|---|
| `IDENT` | the name, `r#name` for a raw identifier |
| `KW` | the keyword (`fn`, `self`, `Self`, `dyn`, ...); `union`, `macro_rules`, `auto` are `IDENT` |
| `LIFETIME` | `'a`, `'static`, `'_` (with the quote) |
| `INT` | `SPELLING =VALUE [big] [SUFFIX]`: the digits without `_` (prefix `0x` `0o` `0b` kept), the low 64 bits as an unsigned decimal, `big` if the value does not fit 64 bits, the suffix |
| `FLOAT` | `SPELLING =VALUE [SUFFIX]`: the spelling without `_` and suffix, the value printed with `%g` |
| `CHAR` | `U+XXXX`, the codepoint |
| `BYTE` | `0xXX` |
| `STR`, `BSTR` | the decoded bytes in double quotes (`\\ \" \n \r \t`, other bytes outside 0x20..0x7E as `\xHH`, so a string is shown as UTF-8 bytes), then ` raw` for `r"..."` / `br"..."` |
| `DOC` | `outer` or `inner`, then the comment text in quotes (without `///`, `//!` or `/** */`) |
| punctuation | the ID from `src/puncts.inc` (`PLUS`, `LBRACE`, `PATHSEP`, ...), then the spelling |

Ordinary comments, whitespace and a leading `#!` line (not `#![`) produce no
token.  With `--tokens-flags` a line ends with ` space` if whitespace or a
comment comes before the token, and ` joint` if the token is punctuation and the
next source char is also a punctuation char (`&` in `&-1`).

## Files

| File | Covers |
|---|---|
| `keywords.rs` | strict, 2018 and reserved keywords, weak keywords as identifiers, raw identifiers, `_` |
| `numbers.rs` | decimal, hex, octal, binary, `_`, all suffixes, values above 64 bits, floats and exponents, `1..2`, `1.foo()`, tuple indices `x.0.1` |
| `chars.rs` | char literals and escapes, byte literals, lifetimes and labels versus chars |
| `strings.rs` | string escapes, line continuation, raw strings with `#`, byte strings, Unicode text |
| `comments.rs` | shebang, nested block comments, outer and inner doc comments (line and block), plain look-alikes (`////`, `/***/`) |
| `inner_attr.rs` | `#![...]` on the first line is not a shebang |
| `punct.rs` | all punctuation, longest match, `space` / `joint` flags |
| `unicode.rs` | Unicode identifiers, emoji and non-Latin text in strings, comments and chars |

To make a new expected file, run the translator and check the output by eye
before committing it:

    obj/rustc-native --tokens tests/lex/x.rs | sed 1,/^Arguments/d > tests/lex/x.tokens
