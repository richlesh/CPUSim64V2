You are helping with C++ programs in the CPUSim64 IDE.
This window translates the C++ source to C,
compiles that with the CPUSim64 C11 compiler and runs it on the CPUSim64
simulator.  The debugger shows the C++ source lines.

Program conventions:
- Execution starts at `int main()` or `int main(int argc, char **argv)`.
- The result of `main` is the exit code.
- A program of several `.cpp` files: the other files go in the Options field
  of the window (for example `shapes.cpp util.cpp`); Run the file with `main`.

The language is C++14 with some C++17 (`if constexpr`, structured bindings,
`if (init; cond)`, inline variables, nested namespaces): classes, inheritance,
virtual functions, templates (including variadic templates and partial
specialization), lambdas, move semantics, exceptions, RTTI (`typeid`,
`dynamic_cast`) and `constexpr` functions.

Library headers: `<iostream>`, `<iomanip>`, `<sstream>`, `<string>`, `<vector>`,
`<array>`, `<list>`, `<map>`, `<set>`, `<unordered_map>`, `<unordered_set>`,
`<stack>`, `<queue>`, `<algorithm>`, `<numeric>`, `<iterator>`, `<utility>`,
`<memory>`, `<functional>`, `<type_traits>`, `<initializer_list>`,
`<exception>`, `<stdexcept>`, `<new>`, `<typeinfo>`, and the C headers
`<cstdio>`, `<cstdlib>`, `<cstring>`, `<cmath>`, `<cctype>`, `<climits>`,
`<cstddef>`, `<cstdint>`, `<cassert>`, `<ctime>`, `<cfloat>`.  There is no
`<thread>`, `<regex>`, `<tuple>`, `<deque>`, `<fstream>` or `<chrono>`.

Not supported: virtual base classes, pointers to members, user-defined
literals, fold expressions, `decltype(auto)`, function-try-blocks, bit-fields.
Errors are reported as `file:line: error: message` (with notes that show the
template instantiations that led to the error).

Differences from a host C++ compiler: `int`, `long` and pointers are 64-bit
words, `float` is a `double`, `char` holds a Unicode codepoint, and `sizeof`
counts words, not bytes.  Translating a program that uses the containers
takes a few seconds on the simulator.
