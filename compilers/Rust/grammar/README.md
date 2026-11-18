# The Rust grammar

`RustLexer.g4` and `RustParser.g4` are unmodified copies of the Rust grammar of
[antlr/grammars-v4](https://github.com/antlr/grammars-v4/tree/master/rust)
(`rust/RustLexer.g4`, `rust/RustParser.g4`). The rustc64 parser is a
hand-written recursive-descent parser that follows their rules.

- Fetched: 2026-11-16 from `raw.githubusercontent.com/antlr/grammars-v4/master/rust/`.
  Latest commit touching `rust/` at that time: `85e2dd1779ce556111ef98ead5d62a9858e95599` (2026-10-21).
- License: MIT. The header of each file (Copyright (c) 2010 The Rust Project
  Developers, Copyright (c) 2020-2022 Student Main) is kept as is.
- The grammar's own README says it follows the Rust Reference, last updated for
  Rust 1.60.0, stable 2018+ features only.
- ANTLR check: both files compile with ANTLR 4.13.2 (`org.antlr.v4.Tool`, output in a
  temp dir) with no errors or warnings. The Java base classes are not
  present, so generating a runnable parser needs stubs for them.

## Semantic predicates (the Java base classes are not needed)

`superClass = RustLexerBase` / `RustParserBase`. They only supply these predicates:

- `SOF()` (lexer): true at the start of the file. Allows `#!shebang` only on the first line.
- `FloatLiteralPossible()` (lexer): false when the previous token is `.`, so that
  `t.0.1` lexes as `t . 0 . 1` and not as `t . 0.1`.
- `FloatDotPossible()` (lexer): lets `1.` be a float only if the next character is
  not `.`, `_` or an identifier start, so `1..2`, `1.foo()` and `1._x` keep the `1`
  an integer.
- `NextGT()` / `NextLT()` (parser, in `shr` / `shl`): `>>` and `<<` are not tokens.
  The grammar builds them from two adjacent `GT` / `LT` tokens, and the predicate
  requires no whitespace between them. This is how `Vec<Vec<u8>>` closes two generic
  lists. rustc64 can lex `>>` as one token and split it when a generic list ends.

## Main parser rules

| Group | Rules |
|---|---|
| Entry | `crate: innerAttribute* item* EOF` |
| Items | `item: outerAttribute* (visItem \| macroItem)`; `visItem: visibility? (module \| externCrate \| useDeclaration \| function_ \| typeAlias \| struct_ \| enumeration \| union_ \| constantItem \| staticItem \| trait_ \| implementation \| externBlock)` |
| Items (detail) | `function_` (`functionQualifiers`, `functionParameters`, `selfParam`, `functionReturnType`), `struct_` (`structStruct`, `tupleStruct`), `enumeration` (`enumItem`, discriminant), `trait_`, `implementation` (`inherentImpl`, `traitImpl`), `associatedItem`, `externBlock`, `useTree` |
| Types | `type_` (with `+` bounds), `typeNoBounds`: `typePath`, `tupleType`, `neverType`, `rawPointerType`, `referenceType`, `arrayType`, `sliceType`, `inferredType` (`_`), `qualifiedPathInType`, `bareFunctionType`, `traitObjectType` (`dyn`), `implTraitType`, `parenthesizedType` |
| Patterns | `pattern` (top-level `\|` alternatives), `patternNoTopAlt`, `patternWithoutRange`: `literalPattern`, `identifierPattern` (`ref`/`mut`/`@`), `wildcardPattern`, `restPattern`, `rangePattern`, `referencePattern`, `structPattern`, `tupleStructPattern`, `tuplePattern`, `groupedPattern`, `slicePattern`, `pathPattern`, `macroInvocation` |
| Expressions | `expression` (one left-recursive rule with labelled alternatives, below), `expressionWithBlock` (block, `unsafe`, `async`, `loop`/`while`/`for`, `if`, `if let`, `match`), `literalExpression`, `pathExpression`, `structExpression`, `closureExpression`, `callParams`, `matchArm` / `matchArmGuard` |
| Statements | `statement: ; \| item \| letStatement \| expressionStatement \| macroInvocationSemi`; `blockExpression: { innerAttribute* statements }`; `statements` ends in an optional tail expression |
| Generics | `genericParams` (`lifetimeParam`, `typeParam`, `constParam`), `genericArgs` (lifetimes, types, const, `Name = Type` bindings), `whereClause` (`lifetimeWhereClauseItem`, `typeBoundWhereClauseItem`, `forLifetimes`), `typeParamBounds`, `traitBound` (`?Sized`, `for<'a>`), `lifetime` |
| Paths | `simplePath`, `pathInExpression` (turbofish `::<`), `typePath` (`Fn(A) -> B` via `typePathFn`), `qualifiedPathInExpression` (`<T as Trait>::x`) |
| Attributes | `outerAttribute: #[attr]`, `innerAttribute: #![attr]`, `attr: simplePath attrInput?`, `attrInput` (delimited token tree or `= expression`) |
| Macros | `macroInvocation: simplePath ! delimTokenTree`, `macroInvocationSemi`, `delimTokenTree` / `tokenTree` / `tokenTreeToken`, `macroRulesDefinition` (`macroRule`, `macroMatcher`, `macroFragSpec`, `macroRepOp`, `macroTranscriber`) |

Expression precedence implied by the order of alternatives in `expression`
(first = binds tightest; ANTLR takes alternative order as precedence, all binary operators are left-associative):

1. postfix: method call, field, tuple index, `.await`, call `()`, index `[]`, `?`
2. prefix: `&` `&&` `&mut`, `*`, `-`, `!`
3. `as`
4. `*` `/` `%`
5. `+` `-`
6. `<<` `>>`
7. `&`
8. `^`
9. `|`
10. comparison `== != > < >= <=` (left-associative here; rustc rejects `a < b < c`)
11. `&&`
12. `||`
13. ranges `..` `..=`
14. `=` and compound assignment (left-associative here; Rust is right-associative)
15. `continue`, `break`, `return` (prefix, take an optional expression)

## Known gaps versus real Rust and what rustc64 will do

- **Macro invocations and token trees.** The grammar parses `path!(...)` into raw
  tokens and never expands them. A macro in expression, statement, item, type or
  pattern position parses, but it has no AST meaning. `macro_rules!` definitions parse
  but are not matched. rustc64: parse invocations into a token tree; implement only
  built-ins (`println!`, `print!`, `format!`, `vec!`, `assert!`, `assert_eq!`, `panic!`)
  by parsing their arguments as expressions; expand simple `macro_rules!` by
  substitution if time allows; otherwise reject with a clear error. Proc macros and `#[derive]` are not supported
  (derive of a few traits may be special-cased).
- **Struct expression ambiguity in conditions.** `structExpression` is an ordinary
  expression alternative, so `if x == S { .. }` is ambiguous (ANTLR resolves it by
  trial, rustc forbids struct literals there). rustc64: keep a "no struct literal"
  flag while parsing the condition of `if`, `while`, `match` scrutinee and the `for`
  iterator; parentheses clear the flag.
- **Closures.** `closureExpression` is accepted, and a closure body extends as far
  as it can. `move`, typed parameters and `-> T { }` bodies are included. rustc64:
  parse `|a, b| expr`, `|| expr`, `|x: T| -> R { }`; no capture analysis beyond
  by-reference and `move`. The lexer's `||` must be split into two `|` for `||` closures.
- **Ranges.** Ranges are plain binary operators, so `a..b` has no lower and upper
  operand restriction, and `..` is allowed as a prefix. rustc64: parse ranges
  non-associatively at the precedence above; `a..` with no end is recognised when the next token cannot begin an expression (`)`, `]`, `}`, `,`, `;`, `{` in conditions).
- **Turbofish.** `pathInExpression` requires `::<`, and `typePath` allows `<` directly.
  rustc64: same split (types after `::` or in type position only; `a < b` in
  expressions is a comparison). `Vec::<i32>::new()` and `x.collect::<Vec<_>>()` must work.
- **Raw and byte strings.** `r"..."`, `r#"..."#`, `b'x'`, `b"..."`, `br#"..."#`
  are lexer tokens, but escapes in ordinary strings are only partly checked, and
  the nesting depth of `#` is unlimited. C-strings (`c"..."`) and string continuation
  (`\` + newline skipping whitespace) are not in this 1.60 grammar. rustc64: lexer
  handles all of them by hand, counting `#` for raw strings.
- **Nested block comments.** The lexer rule is recursive (`BLOCK_COMMENT_OR_DOC`),
  so nesting parses. Doc comments (`///`, `//!`, `/** */`, `/*! */`) are
  hidden-channel tokens and do not become `#[doc]` attributes. rustc64: keep a depth counter and
  drop doc comments.
- **Lifetimes versus char literals.** `LIFETIME_OR_LABEL` is `'` + identifier and
  `CHAR_LITERAL` is `'x'`; ANTLR picks the longest match, so `'a'` is a char and
  `'a` a lifetime. `'static` and `'_` are separate tokens. rustc64: after `'`, if
  the char after the next one is `'` (or the next is `\`), it is a char literal; otherwise a lifetime or label.
  Loop labels (`'outer: loop`) and `break 'outer` use the same token.
- **Numeric suffixes.** `INTEGER_SUFFIX` (`u8`..`u128`, `i8`..`i128`, `usize`, `isize`)
  and `FLOAT_SUFFIX` (`f32`, `f64`) are part of the literal token; underscores are
  allowed; `1e3`, `2.5E-3`, `0x`, `0o`, `0b` are covered. A suffix on a float-looking
  integer (`1f32`) and `1.` followed by a method are handled by predicates. rustc64:
  scan digits first, then the suffix; `1.` is a float only if not followed by `.` or an identifier start; tuple
  indices (`t.0.1`) must be lexed as separate integers.
- **Attributes.** `#[...]` and `#![...]` carry a path and a raw token tree or
  `= expression`; the grammar does not check names. Attributes on expressions,
  statements, fields, parameters and match arms are only partly allowed. rustc64:
  parse and keep attributes on items; honour `#[derive]`, `#[allow]`, `#[inline]`,
  `#[test]`, `#[cfg(...)]` (evaluate simple `cfg` predicates), and ignore the rest with a warning.
- **Other gaps.** The grammar is 2018 edition only: no `let ... else`, no `let` chains,
  no GAT or `impl Trait` in more positions, no const generics defaults, no
  `async` closures, and no raw `&raw` borrows. Keywords `union`, `macro_rules`,
  `auto`, `default` are contextual and the grammar has dedicated tokens for some of them, so
  `union` as an identifier needs care. Semantic checks (types, borrows,
  lifetimes, name resolution) are outside any grammar.
