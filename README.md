# R# (Rosary Sharp)

A compiled language with Python-style syntax and C-level speed. File extension `.rsh`, CLI `rsharp` (alias `rsh`). The compiler is written in C++20 and transpiles to C11.

```
fn fib(n: int) -> int:
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)

fn main():
    for i in 0..10:
        print("fib({i}) = {fib(i)}")
```

Status: **early development. M0 (spec) done, M1 lexer written; working toward the first running program.** See `docs/spec-v0.1.md`.

## Layout

| Folder | Purpose |
|---|---|
| `compiler/` | The compiler (C++): source files, diagnostics, tokens; lexer, parser, type checker, C codegen to come |
| `runtime/` | C runtime linked into every R# program (refcounting, strings, lists, maps) |
| `stdlib/` | Standard library written in R# |
| `examples/` | Sample `.rsh` programs (also used as tests later) |
| `tests/` | C++ unit tests (CTest) and, later, end-to-end program tests |
| `vscode-ext/` | VS Code extension: syntax highlighting, then language server |
| `docs/` | Language spec and design notes |

## Build

Requires CMake 3.20+, a C++20 compiler (GCC 11+, Clang 14+, or MSVC 2022), and optionally Ninja.

```
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
./build/rsharp version
```

On Windows (Developer PowerShell): `cmake -S . -B build` then `cmake --build build --config Debug`, and run `build\Debug\rsharp.exe version`.

In VS Code, install the recommended extensions, then use **Terminal > Run Build Task** (Ctrl+Shift+B) and **Run Test Task**.

## Roadmap

M1 lexer, M2 parser, M3 type checker, M4 C backend + runtime, M5 collections/strings/Result, M6 CLI `build`/`run`, M7 VS Code highlighting, M8 language server, M9 Python interop. Details in the spec, section 15.
