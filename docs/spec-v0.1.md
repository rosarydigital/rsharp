# R# (Rosary Sharp) — Language Spec, Draft v0.1

> Status: DRAFT v0.1.1. Open questions 1, 2, 4, 5, 6 are decided (see section 14); only integer overflow is still open (defaulted).
> File extension: `.rsh` · CLI: `rsharp` (short alias `rsh`) · Compiler written in C++ · Backend: transpile to C11

## 1. Goals

1. **Python-like writing experience**: indentation blocks, no semicolons, no headers, type inference for locals, string interpolation, built-in list/map/string.
2. **C-like speed**: statically typed, compiled ahead of time to native code via C, no interpreter, no tracing garbage collector.
3. **Direct C interop**: call any C library with a one-line declaration.
4. **Small enough to build**: v0.1 deliberately leaves out features (user generics, classes, exceptions, async) so the compiler is achievable by one person.

Non-goals for v0.1: self-hosting, package manager, multithreading, user-defined generics, operator overloading, a borrow checker.

## 2. Lexical structure

- **Source**: UTF-8 text. Line endings `\n` or `\r\n`.
- **Comments**: `# line comment`. Block/doc comments: `## ... ##` (optional, post-v0.1).
- **Blocks**: introduced by `:` at the end of a line, followed by an indented block. Indent is **4 spaces**; tabs are a compile error. Lexer emits `INDENT` / `DEDENT` / `NEWLINE` tokens (like Python's tokenizer).
- **Line continuation**: inside `()`, `[]`, `{}` newlines are ignored.
- **Identifiers**: `[A-Za-z_][A-Za-z0-9_]*`. Convention: `snake_case` for values/functions, `PascalCase` for types.
- **Keywords**: `fn let var if elif else while for in return break continue struct enum match import extern pub and or not true false none ok err unsafe as`
- **Literals**:
  - ints: `42`, `1_000_000`, `0xFF`, `0b1010`
  - floats: `3.14`, `1e-9`
  - bool: `true`, `false`
  - char: `'a'`
  - string: `"hello"`, with interpolation `"x = {x}"`, format specs `"{x:.2f}"`, and escapes `\n \t \" \\ \{`
  - raw string: `r"C:\path"`

## 3. Types

### 3.1 Primitives
| Type | Meaning |
|---|---|
| `int` | 64-bit signed (alias of `i64`) |
| `i8 i16 i32 i64` | signed ints |
| `u8 u16 u32 u64` | unsigned ints |
| `float` | 64-bit float (alias of `f64`) |
| `f32 f64` | floats |
| `bool` | true/false |
| `char` | one Unicode scalar value |
| `none` | the unit type (no value) |

Integer overflow: **aborts with a message in debug builds, wraps (two's complement) in release builds**. This is a default chosen for you; it is the only open question left (see section 14).
No implicit numeric conversions; use `as`: `let x = n as float`.

### 3.2 Built-in heap types (reference counted)
- `string` — immutable UTF-8, `len()`, slicing `s[1..4]`, `+` concatenation, interpolation.
- `list[T]` — growable array. `[1, 2, 3]`, `xs[i]`, `xs.push(v)`, `xs.pop()`, `len(xs)`, slices.
- `map[K, V]` — hash map. `{"a": 1}`, `m[k]`, `m.get(k)` returns `Option[V]`, `m.has(k)`, `m.remove(k)`.
- `Option[T]` — `some(v)` or `none`.
- `Result[T, E]` — `ok(v)` or `err(e)`.

Generics are **hardcoded in the compiler** for these types in v0.1. The compiler monomorphizes each used instance (`list[int]` generates its own C struct and functions). Users cannot declare their own generic types/functions yet.

### 3.3 User-defined types
```
struct Point:
    x: float
    y: float

enum Shape:
    Circle(radius: float)
    Rect(w: float, h: float)
    Empty
```
- Structs are **value-like fields in a refcounted heap object** (see §5).
- **Methods** are declared in `impl` blocks and called with dot syntax (`p.length()`). The receiver is the implicit first parameter `self`:
  ```
  impl Point:
      fn length(self) -> float:
          return sqrt(self.x * self.x + self.y * self.y)

      fn scale(self, k: float) -> Point:
          return Point(self.x * k, self.y * k)
  ```
  Method calls compile to plain C functions (`Point_length(p)`), so there is no virtual dispatch or runtime cost. No inheritance in v0.1. Built-in types get methods the same way (`xs.push(v)`, `s.split(" ")`).
- `match` works on enums, `Option`, and `Result`.

### 3.4 Type inference
- Locals infer from the initializer: `let n = 10` is `int`; `let xs = [1.0, 2.0]` is `list[float]`.
- Function parameters and return types **must be annotated**.
- Empty collections need an annotation: `let xs: list[int] = []`.

## 4. Declarations and statements

```
let name = "Rosary"        # immutable binding
var count = 0              # mutable binding
count += 1

fn add(a: int, b: int) -> int:
    return a + b

if x > 0:
    print("positive")
elif x == 0:
    print("zero")
else:
    print("negative")

while n > 0:
    n -= 1

for i in 0..10:            # 0 to 9, half-open
    print(i)
for i in 0..=10:           # inclusive
    print(i)
for item in items:         # any list
    print(item)
for k, v in my_map:
    print("{k}: {v}")
```

- Operators: `+ - * / % ** == != < <= > >= and or not`, bitwise `& | ^ ~ << >>`, assignment `= += -= *= /= %=`.
- Division is **Python-style**: `/` always produces a `float` (`7 / 2 == 3.5`), `//` is floor division on ints (`7 // 2 == 3`), `%` is the matching modulo (result has the sign of the divisor, as in Python). `**` is exponentiation.
- Entry point: `fn main():` or `fn main() -> int:` (exit code).
- Top-level statements other than declarations are not allowed (no script mode in v0.1).

### 4.1 Format specs in interpolation

`{expr:spec}` follows Python's format mini-language, a subset in v0.1:

```
let pi = 3.14159265
print("{pi:.2f}")        # 3.14
print("{42:5d}|")        # "   42|"
print("{42:<5d}|")       # "42   |"
print("{42:05d}")        # 00042
print("{255:x}")         # ff
print("{255:#b}")        # 0b11111111
print("{0.256:.1%}")     # 25.6%
print("{big:,}")         # 1,000,000
```

Spec grammar: `[[fill]align][sign][#][0][width][,][.precision][type]` where align is `<`, `>`, `^`; type is one of `d x X o b e f g %  s`. The compiler **checks the spec against the expression's type at compile time** (for example `.2f` on a string is a compile error) and lowers it to a call into the runtime formatter. `{{` and `}}` produce literal braces (in addition to `\{`).

## 5. Memory model: scoped ownership + reference counting

**Rules**
1. Primitives live on the stack and are copied.
2. Every heap value (`string`, `list`, `map`, `struct`, `enum` with data) has a **reference count**.
3. Assigning or passing a heap value **shares it** (count +1). It does not deep copy.
4. When a binding goes out of scope, its count is decremented (-1). At zero, the object is freed immediately and deterministically. No GC pauses.
5. `.copy()` makes an explicit deep copy.
6. Function parameters are **borrowed by default**: the callee does not increment the count unless it stores the value somewhere that outlives the call. The compiler elides increments/decrements where it can prove they are unnecessary (a key optimization, post-v0.1).
7. `move x` transfers ownership without touching the count (after a move, `x` cannot be used; the compiler checks this).
8. **Cycles are not collected.** Provide `weak[T]` references (post-v0.1) for parent pointers, and document the limitation.

**Mutation semantics** (decided: sharing): because sharing is the default, `let b = a` for a list means both names see the same list, as in Python. Mutating through `var`-declared lists is visible to all holders. A `let` binding prevents rebinding **and** mutating through that name.

**Implementation**: refcounts are non-atomic in v0.1 (single-threaded). Threads come later with atomic counts or ownership transfer only.

## 6. Error handling

No exceptions. Fallible functions return `Result[T, E]`:

```
fn parse_port(s: string) -> Result[int, string]:
    let n = to_int(s)?          # `?` returns early with the err if it failed
    if n < 1 or n > 65535:
        return err("port out of range")
    return ok(n)

fn main():
    match parse_port("8080"):
        ok(p):  print("port {p}")
        err(e): print("error: {e}")
```

- `?` works on `Result` (in functions returning a compatible `Result`) and `Option`.
- Runtime errors that are bugs (index out of bounds, null deref in unsafe, divide by zero) **abort** with a message and source line.
- `panic("message")` is available.

## 7. Modules

- One file = one module. `import math` loads `math.rsh` from the project or the standard library.
- `pub fn` / `pub struct` marks exports. Everything else is private to the file.
- `import math as m`, `from math import sqrt, pi`.
- Circular imports are a compile error in v0.1.

## 8. C interoperability

```
import c "stdio.h"
import c "math.h"

extern fn puts(s: cstr) -> i32
extern fn sqrt(x: f64) -> f64

fn main():
    let r = sqrt(2.0)
    print("sqrt 2 = {r}")
    unsafe:
        puts("hello from C".c_str())
```

- `import c "header.h"` adds the header to the generated C and makes it linkable.
- `extern fn` declares the signature R# should use for a C function.
- C types: `cstr` (`const char*`), `ptr[T]` (raw pointer), `i32`, `u8`, etc.
- Raw pointer operations (`*p`, pointer arithmetic, calling functions that take `ptr`) are allowed only inside `unsafe:` blocks.
- Link libraries from `rsharp.toml`: `link = ["m", "sqlite3"]`.
- Later: `rsharp bindgen header.h` to auto-generate `extern` declarations.

## 9. Compilation model

```
.rsh files
  → Lexer (indent-aware) → Parser → AST
  → Name resolution → Type checking + inference
  → Monomorphization of built-in generics
  → Lowering to simple IR (ownership/refcount insertion happens here)
  → C11 code generator → out.c + rsharp_runtime.h/.c
  → clang / gcc / zig cc → native executable
```

- **Runtime library** (`rsharp_runtime`): refcounting, strings, lists, maps, panic/abort, print.
- **Debug mapping**: emit `#line N "file.rsh"` so C compiler errors and debuggers point at R# source.
- **Build modes**: `rsharp build` (debug: checks on, `-O0 -g`), `rsharp build --release` (`-O2`, wrapping ints).
- **Windows**: bundle or auto-detect a C compiler (zig cc / clang). Linux: use system clang/gcc.
- **Compiler errors**: Rust/Elm-style messages with file, line, the offending source line, and a caret.

## 10. Standard library (v0.1 scope)

`print`, `input`, `len`, `range`, string methods (`split`, `join`, `trim`, `replace`, `find`, `upper`, `lower`, `starts_with`), `math` (`sqrt`, `pow`, `sin`, `cos`, `pi`, ...), `io` (read/write files, returning `Result`), `os` (args, env, exit), `time`.

## 11. Python library interop (future, honest scope)

Compiling arbitrary Python libraries to C is not realistic: Python is dynamically typed, and most popular libraries (NumPy, pandas, etc.) are C extensions built on the CPython runtime. Realistic paths, in increasing effort:

1. **Embed CPython and call it from R#** (`import py "numpy"`): works with almost every library, at Python speed for those calls. Convert values at the boundary.
2. **Typed stubs**: use `.pyi` stub files to give the compiler type signatures for the Python calls, so wrappers are generated automatically and type-checked.
3. **Subset translator**: a `py2rsh` tool that converts *typed*, simple Python (annotated functions, no metaprogramming) into R#. Useful for pure-Python helper code, not whole libraries.

Plan: build #1 and #2 after the core language works. #3 is a stretch goal.

## 12. Example programs

**Hello**
```
fn main():
    print("Hello from R#!")
```

**Fibonacci**
```
fn fib(n: int) -> int:
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)

fn main():
    for i in 0..10:
        print("fib({i}) = {fib(i)}")
```

**Word counts**
```
fn main():
    let text = "the cat and the hat and the bat"
    var counts: map[string, int] = {}
    for word in text.split(" "):
        counts[word] = (counts.get(word) or 0) + 1
    for word, n in counts:
        print("{word}: {n}")
```

**Structs and enums**
```
struct Point:
    x: float
    y: float

enum Shape:
    Circle(radius: float)
    Rect(w: float, h: float)

fn area(s: Shape) -> float:
    match s:
        Circle(r):  return 3.14159 * r * r
        Rect(w, h): return w * h

fn main():
    let shapes = [Circle(2.0), Rect(3.0, 4.0)]
    for s in shapes:
        print("area = {area(s)}")
```

**Reading a file with error handling**
```
import io

fn first_line(path: string) -> Result[string, string]:
    let text = io.read_file(path)?
    let lines = text.split("\n")
    if len(lines) == 0:
        return err("empty file")
    return ok(lines[0])

fn main():
    match first_line("data.txt"):
        ok(line): print(line)
        err(e):   print("failed: {e}")
```

## 13. Grammar sketch (EBNF)

```
program      = { import_stmt | decl } ;
import_stmt = "import" ( "c" STRING | module ["as" NAME] )
             | "from" module "import" NAME {"," NAME} ;
decl         = fn_decl | struct_decl | enum_decl | extern_decl | let_decl ;
fn_decl      = ["pub"] "fn" NAME "(" [params] ")" ["->" type] ":" block ;
params       = NAME ":" type { "," NAME ":" type } ;
struct_decl  = ["pub"] "struct" NAME ":" NEWLINE INDENT { NAME ":" type NEWLINE } DEDENT ;
enum_decl    = ["pub"] "enum" NAME ":" NEWLINE INDENT { variant NEWLINE } DEDENT ;
extern_decl  = "extern" "fn" NAME "(" [params] ")" ["->" type] NEWLINE ;
block        = NEWLINE INDENT { statement } DEDENT ;
statement    = let_stmt | assign | if_stmt | while_stmt | for_stmt
             | match_stmt | return_stmt | break | continue | unsafe_block | expr NEWLINE ;
let_stmt     = ("let" | "var") NAME [":" type] "=" expr NEWLINE ;
if_stmt      = "if" expr ":" block { "elif" expr ":" block } ["else" ":" block] ;
while_stmt   = "while" expr ":" block ;
for_stmt     = "for" NAME ["," NAME] "in" expr ":" block ;
match_stmt   = "match" expr ":" NEWLINE INDENT { pattern ":" (statement | block) } DEDENT ;
type         = NAME | NAME "[" type {"," type} "]" ;
expr         = standard precedence climbing: or / and / not / compare / range
               / add / mul / unary / power / postfix(call, index, member, "?") / atom ;
```
(Full precedence table to be written in the next revision.)

## 14. Decisions log

| # | Question | Decision |
|---|---|---|
| 1 | Aliasing | **Share** (Python-style). `let b = a` shares the object; `.copy()` for a deep copy. |
| 2 | Methods | **Yes in v0.1**: `impl` blocks and `p.length()` dot syntax (section 3.3). |
| 3 | Integer overflow | **Open, defaulted**: abort in debug, wrap in release. Change any time before M3. |
| 4 | Division | **Python-style**: `/` is always float, `//` is floor division. |
| 5 | Formatting | **Format specs supported**: `{x:.2f}` etc. (section 4.1). |
| 6 | CLI name | **`rsharp`**, with `rsh` as a short alias. File extension stays `.rsh`. |

Also decided earlier: indentation blocks, scoped ownership + reference counting, built-in string/list/map, hardcoded generics, `Result` for errors, direct C calls, transpile-to-C backend first, compiler in C++, Windows first then Linux.

### Still to decide later (not blocking v0.1)
- Cycle handling: `weak[T]` design.
- Whether `match` arms need exhaustiveness checking in v0.1 (recommended: yes).
- Package manager and `rsharp.toml` dependency format.
- Threading model.

## 15. Milestones

| # | Milestone | Done when |
|---|---|---|
| M0 | Spec v0.1 approved | open questions answered |
| M1 | Lexer | tokenizes all examples incl. INDENT/DEDENT |
| M2 | Parser | produces AST for all examples; good syntax errors |
| M3 | Type checker | inference + errors; rejects bad programs |
| M4 | C backend + runtime | hello, fib, structs run natively |
| M5 | Collections + strings + Result | word-count and file examples run |
| M6 | CLI `rsharp build/run` | works on Windows and Linux |
| M7 | VS Code: highlighting + tasks | `.rsh` files colored; F5/build works |
| M8 | Language server | diagnostics, hover, go-to-def |
| M9 | Python interop (CPython embed) | `import py "..."` works |
