# C++ translator for CPUSim64 (`compilers/cpp`)

Runs C++14 programs (with some C++17) in the CPUSim64 IDE (File → New Compiler → cpp).
The translator `cppc` is written in portable C and compiled by the CPUSim64 C11
compiler; it translates `prog.cpp` to `prog.gen.c`, with `#line` directives that
name `prog.cpp`.  The IDE then runs the C11 compiler on `prog.gen.c`
(`translate_to: C11` in `config.properties`) and uses the result as `prog.asm`,
so the debugger shows the C++ lines.

See `plan.md` for the design, the task list and the measurements.

## Supported

- Classes: constructors and destructors, copy and move semantics, operator
  overloading, conversions, access control, friends, static members, nested
  classes, single and multiple inheritance, virtual functions, abstract
  classes, `override` / `final`, delegating and inheriting constructors,
  `= default` / `= delete`.
- Templates: function, class, member and alias templates, variable templates,
  default arguments, non-type parameters, explicit and partial specialization,
  argument deduction, SFINAE (`enable_if`), variadic templates (`sizeof...`, pack
  expansions), instantiation context in error messages.
- C++11/14: `auto` (also return types), `decltype`, range-for over arrays,
  braced lists and containers, `nullptr`, lambdas (captures by value and
  reference, init-captures, `mutable`, generic lambdas, conversion to a
  function pointer), rvalue references, `std::move` / `std::forward`,
  `std::initializer_list`, uniform initialization, `constexpr` functions (evaluated
  at translation time), `static_assert`, `enum class`, placement new.
- Exceptions: `throw`, `try` / `catch` by type, by reference and `catch (...)`,
  rethrow, stack unwinding with destructors (also of partly built objects),
  `std::terminate` for an uncaught exception.
- RTTI: `typeid`, `std::type_info`, `dynamic_cast` for pointers and references
  (down and cross casts), `std::bad_cast`.
- C++17: `if constexpr`, structured bindings, `if` / `switch` with an initializer,
  inline variables, nested namespace definitions, `[[nodiscard]]` and
  `[[maybe_unused]]` (accepted).
- Programs of several `.cpp` files: put the other files in the Options field
  (see below).

The library (`lib/`): `<iostream>`, `<iomanip>`, `<sstream>`, `<string>`,
`<vector>`, `<array>`, `<list>`, `<map>`, `<set>`, `<unordered_map>`,
`<unordered_set>`, `<stack>`, `<queue>`, `<algorithm>`, `<numeric>`,
`<iterator>`, `<utility>`, `<memory>` (`unique_ptr`, `shared_ptr`, `weak_ptr`),
`<functional>` (`std::function`, function objects, `std::hash`), `<type_traits>`,
`<initializer_list>`, `<exception>`, `<stdexcept>`, `<new>`, `<typeinfo>`, and
the C headers `<cstdio>`, `<cstdlib>`, `<cstring>`, `<cmath>`, `<cctype>`,
`<climits>`, `<cstddef>`, `<cstdint>`, `<cassert>`, `<ctime>`, `<cfloat>`.

Not supported (reported as an error): virtual base classes, pointers to
members, user-defined literals, fold expressions, `decltype(auto)`,
function-try-blocks, bit-fields, variable length arrays.  Templates are
checked when they are instantiated, not when they are defined.

Differences from g++: `int`, `long` and pointers are 64-bit words, `float` is a
`double`, `char` holds a Unicode codepoint, `sizeof` counts words,
`type_info::name()` is the C++ spelling of the type (not a mangled name).

## Several source files

In the IDE, write the other `.cpp` files of the program in the window's
Options field (`shapes.cpp util.cpp`), and Run the file with `main()`.  The IDE
translates each file (the others with `cppc --unit`), compiles the C files as
modules of one program (`cc main.gen.c shapes.gen.c util.gen.c -lstd`) and links
them.  Functions defined in a class or `inline`, template instances and the
library are compiled into every unit that uses them (with internal linkage).

## Building

```
./build.sh            # cppc.asm, compiled by the C11 compiler (committed)
./build.sh --native   # obj/cppc-native, for development with the host cc
```

## Tests

```
tests/run_tests.sh [filter]          # translator, C11 and the simulator
CPPC=native tests/run_tests.sh       # with the native build of the translator
SIM=java tests/run_tests.sh          # run the programs on the Java simulator
HOST=1 tests/run_tests.sh            # also check .expected against g++ -std=c++14
tests/antlr_check.sh                 # the parser against the ANTLR CPP14 grammar
```

A run test `NAME.cpp` has `NAME.expected` and optionally `NAME.stdin`,
`NAME.argv`, `NAME.exit`, `NAME.std` (the g++ standard, e.g. `c++17`) and
`NAME.units` (the other source files of the program).

## Layout

| Path | Purpose |
|---|---|
| `src/` | the translator (one translation unit, `cppc.c` includes the others) |
| `lib/` | the C++ standard library subset |
| `grammar/` | the ANTLR CPP14 grammar the parser follows |
| `tests/` | `dump/`, `errors/`, `run/` regression tests, the ANTLR check |
