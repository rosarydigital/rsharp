// Minimal test harness. Each TEST registers itself; main runs them all.

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "diagnostics.h"
#include "source.h"
#include "token.h"

namespace {

struct TestCase {
    const char* name;
    std::function<void()> fn;
};
std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}
struct Registrar {
    Registrar(const char* n, std::function<void()> f) { registry().push_back({n, std::move(f)}); }
};
int g_failures = 0;

#define TEST(name)                                  \
    static void name();                             \
    static Registrar reg_##name(#name, name);       \
    static void name()

#define CHECK(cond)                                                                \
    do {                                                                           \
        if (!(cond)) {                                                             \
            ++g_failures;                                                          \
            std::cerr << "  CHECK failed: " #cond " (line " << __LINE__ << ")\n";  \
        }                                                                          \
    } while (0)

using namespace rsharp;

TEST(source_line_lookup) {
    SourceFile f("t.rsh", "fn main():\n    print(1)\r\nlast");
    CHECK(f.line_text(1) == "fn main():");
    CHECK(f.line_text(2) == "    print(1)");  // \r stripped
    CHECK(f.line_text(3) == "last");
    CHECK(f.line_text(4).empty());
}

TEST(keywords_recognized) {
    CHECK(keyword_or_identifier("fn") == TokenKind::KwFn);
    CHECK(keyword_or_identifier("elif") == TokenKind::KwElif);
    CHECK(keyword_or_identifier("impl") == TokenKind::KwImpl);
    CHECK(keyword_or_identifier("rosary") == TokenKind::Identifier);
}

TEST(diagnostic_format) {
    SourceFile f("hello.rsh", "fn main():\n\tprint(1)\n");
    DiagnosticSink sink(f);
    sink.error(Span{12, 1, 2, 1}, "tabs are not allowed for indentation", "use 4 spaces");
    CHECK(sink.has_errors());
    std::ostringstream out;
    sink.print(out);
    CHECK(out.str().find("error: tabs are not allowed") != std::string::npos);
    CHECK(out.str().find("hello.rsh:2:1") != std::string::npos);
    CHECK(out.str().find("= help: use 4 spaces") != std::string::npos);
}

}  // namespace

int main() {
    for (auto& t : registry()) {
        int before = g_failures;
        t.fn();
        std::cout << (g_failures == before ? "[ ok ] " : "[FAIL] ") << t.name << "\n";
    }
    std::cout << (g_failures == 0 ? "all tests passed" : "some tests FAILED") << "\n";
    return g_failures == 0 ? 0 : 1;
}
