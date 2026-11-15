# VB.NET grammar

`vb.g4` is the grammar of the Visual Basic Language Specification,
version 11, unchanged, as linked from
[dotnet/vblang `spec/README.md`](https://github.com/dotnet/vblang/blob/main/spec/README.md)
(downloaded from `http://ljw1004.github.io/vbspec/vb.g4`; Apache License 2.0,
see the `NOTICE` file at the top of the repository).  The compiler's
recursive-descent parser (`src/`) follows its rules and uses the rule names
in comments.

## Grammar spike (Task 1): `vb.g4` cannot drive ANTLR as it is

ANTLR 4.13.2 processes `vb.g4` with warnings only, but the generated
recognizer cannot parse VB programs:

1. **Every rule is a lexer rule.**  All 2,559 lines use capitalised names
   (`CompilationUnit`, `Statement`, `Expression`, ...), which ANTLR treats
   as lexer rules.  The only parser rule is `start: Start;`.
2. **74 placeholder literals** stand for character classes, for example
   `'<Any Unicode character except a LineTerminator>'`,
   `'<Unicode classes Lu,Ll,Lt,Lm,Lo,Nl>'` and `'<Unicode 0x2028>'`.  ANTLR
   reads them as literal strings.
3. **Keywords overlap.**  `Keyword` lists every reserved word, so tokens
   such as `True`, `ElseIf`, `Public` and `REM` are unreachable as the
   more specific tokens (ANTLR warning 184, about 40 cases).  `&` overlaps
   `LongTypeCharacter`.
4. **Case-insensitivity** is stated in the specification text, not in the
   grammar.
5. **Logical lines.**  The grammar uses `LineTerminator` and
   `LineContinuation`, but the implicit line continuation of VB 10 (after
   `,` `(` `{` binary operators, `:=`, before `)` `}`, around query
   operators, and so on) is described only in the text.
6. **Contextual keywords** (`Aggregate`, `Ansi`, `Assembly`, `Async`, `Auto`,
   `Binary`, `Compare`, `Custom`, `Distinct`, `Equals`, `Explicit`, `From`,
   `Group`, `Infer`, `Into`, `Join`, `Key`, `Off`, `Order`, `Preserve`,
   `Skip`, `Strict`, `Take`, `Text`, `Unicode`, `Until`, `Where`, ...) are
   identifiers except in certain positions.
7. **Language additions after version 11** are missing: string
   interpolation (`$"..."`), `?.` and `?(`, `NameOf`, tuples, and
   multi-line string literals.
8. Some productions are written for reading, not parsing: left recursion
   through several rules in `Expression`, and `RegularInitializer`/
   `ArrayElementInitializer` ambiguities.

## The derived grammar

`VBLexer.g4` and `VBParser.g4` are the cleaned-up grammar.  They describe the
language that the compiler's lexer (`src/lexer.c`) and parser
(`src/parser.c`) accept, and the comment at the top of each file lists
every change from `vb.g4`:

- Lexer (section 2): `options { caseInsensitive = true; }`; one token per
  reserved keyword (generated from `src/keywords.inc`) and per contextual
  keyword (`From`, `Where`, `Strict`, ...), which the parser rule
  `identifier` also accepts as names; real character classes (`\p{Lu}` and
  so on) for the placeholders; `NEWLINE` tokens for line ends; `_` line
  continuation, comments and directive lines skipped.
- Parser (sections 4 to 11): lower-case rule names, one left-recursive
  `expression` rule with VB precedence, `terms` for StatementTerminator,
  the shared `Next j, i` written out (`forShared`), MustOverride members
  split from members with a body.
- Not in the grammar because the compiler does not support them: XML
  literals, `Async`/`Await`/`Iterator`/`Yield`, `Declare`,
  `On Error`/`Resume`/`Error`, `GoSub`, `Let`/`Set`, date literals.
- `EXT:` marks the additions after version 11: interpolated strings,
  `?.` and `?(`, `NameOf`, `&B` literals, digit separators, multi-line
  strings, comments after `_`.

Three rules of the compiler's lexer need context, so the test driver
`tests/antlr/VBCheck.java` applies them as a token filter between the
ANTLR lexer and parser (grammars-v4 grammars have no actions):

- `!` and `&` directly after a name are its type character (`x!`, `x&`);
- implicit line continuation: a line end after `,` `(` `{` `.` `=` `:=`,
  a binary or compound assignment operator, `Is`, `IsNot`, `Like`, `In`,
  or before `)` and `}`, is dropped, as `join_lines()` in `src/lexer.c`
  does;
- the holes of an interpolated string are parsed as expressions (rule
  `holeExpression`).

## Checking the compiler against the grammar

```
tests/antlr_check.sh            # with the CPUSim64 build of the compiler
VBC=native tests/antlr_check.sh # with the native development build
```

It builds the recognizer with ANTLR 4.13.2 from the local Maven repository
and checks that ANTLR and `vbc --parse-only` both accept
`tests/grammar/ok/*.vb`, `tests/dump/ast_*.vb` and `tests/run/*.vb`, and both
reject `tests/grammar/bad/*.vb` and `tests/errors/parse_*.vb`.

Known differences, kept out of the test files: conditional compilation
(`#If`) is not evaluated by the ANTLR lexer, integer literals that overflow
are rejected only by the compiler, and a `_` continuation needs white
space before it in the ANTLR lexer.
