# The C++ grammar

`CPP14Lexer.g4` and `CPP14Parser.g4` are copies of the C++14 grammar of
[antlr/grammars-v4](https://github.com/antlr/grammars-v4/tree/master/cpp)
(`cpp/CPP14Lexer.g4`, `cpp/CPP14Parser.g4`).  The translator's parser
(`src/parser.c`) is a hand-written recursive-descent parser that follows its
rules; `tests/antlr_check.sh` builds the grammar with ANTLR 4.13.2 and checks
that both parsers accept and reject the same programs
(`tests/grammar/ok`, `tests/grammar/bad`, `tests/dump/ast_*.cpp`,
`tests/errors/parse_*.cpp`).  The input files must not use `#include` or
macros, because ANTLR does not run a preprocessor.

`CPP14ParserBase.java` (in `tests/antlr/`) is the grammar's Java base class.

## Where the hand-written parser differs

The grammar is not semantic.  It accepts everything that is syntactically
possible and leaves the choice between a declaration and an expression
(`a * b;`, `a < b > c`, `T(x)`) to the order of its alternatives.  C++ needs
the meaning of names for this, so the parser keeps a symbol table of types,
templates and namespaces (`Sym`, `Scope` in `parser.c`), and:

- a name that is not a known type is an expression, except where only a
  declaration is possible (`Unknown x;`);
- `name <` starts template arguments only after a known template name, after
  the `template` keyword, or after a `.` / `->` member name that is a member
  template somewhere and is followed by arguments and `(` or `::`;
- a qualified name through a dependent qualifier (`T::x`) is a type only after
  `typename`;
- the bodies of member functions are parsed after the class.

Constructs that the grammar accepts and the parser rejects (they are listed
in `tests/grammar/allow.txt`):

- more than one type specifier (`struct S {...} int y;`, `int a int b;`): the
  grammar accepts any sequence of decl-specifiers.

Constructs that the grammar rejects but the parser accepts (not in the test
corpus, because the comparison would fail):

- an abstract array declarator in a type-id (`sizeof(int[3])`, `void f(int [3])`);
- `struct alignas(8) S {...}`;
- a cast to a pointer to array (`(P (*)[3])x`).

Not supported: `decltype` in a destructor name, `explicit(bool)`, and the
C++17/20 syntax apart from `if constexpr` and nested namespace names (which
come in task 15).
