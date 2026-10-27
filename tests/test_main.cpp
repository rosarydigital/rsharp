// Minimal test harness. Each TEST registers itself; main runs them all.

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "diagnostics.h"
#include "lexer.h"
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

// ---------------------------------------------------------------- lexer

using K = TokenKind;

// Lexes `text` and returns just the token kinds; `errors` gets the error count.
std::vector<K> lex_kinds(const std::string& text, size_t* errors = nullptr) {
    SourceFile f("t.rsh", text);
    DiagnosticSink sink(f);
    Lexer lexer(f, sink);
    std::vector<K> kinds;
    for (const Token& t : lexer.tokenize()) kinds.push_back(t.kind);
    if (errors) *errors = sink.error_count();
    return kinds;
}

size_t lex_errors(const std::string& text) {
    size_t errors = 0;
    lex_kinds(text, &errors);
    return errors;
}

TEST(lexer_indent_dedent) {
    size_t errors = 99;
    auto kinds = lex_kinds("fn f():\n    if x:\n        y\nz\n", &errors);
    std::vector<K> want = {K::KwFn, K::Identifier, K::LParen, K::RParen, K::Colon, K::Newline,
                           K::Indent, K::KwIf, K::Identifier, K::Colon, K::Newline,
                           K::Indent, K::Identifier, K::Newline,
                           K::Dedent, K::Dedent, K::Identifier, K::Newline, K::Eof};
    CHECK(kinds == want);
    CHECK(errors == 0);
}

TEST(lexer_dedents_at_eof_without_trailing_newline) {
    auto kinds = lex_kinds("a:\n    b:\n        c");
    std::vector<K> want = {K::Identifier, K::Colon, K::Newline, K::Indent, K::Identifier, K::Colon,
                           K::Newline, K::Indent, K::Identifier, K::Newline, K::Dedent, K::Dedent, K::Eof};
    CHECK(kinds == want);
}

TEST(lexer_blank_and_comment_lines_ignored) {
    auto kinds = lex_kinds("a:\n\n    # comment\n    b  # trailing\n");
    std::vector<K> want = {K::Identifier, K::Colon, K::Newline, K::Indent, K::Identifier,
                           K::Newline, K::Dedent, K::Eof};
    CHECK(kinds == want);
}

TEST(lexer_crlf_line_endings) {
    size_t errors = 99;
    auto kinds = lex_kinds("a:\r\n    b\r\n", &errors);
    std::vector<K> want = {K::Identifier, K::Colon, K::Newline, K::Indent, K::Identifier,
                           K::Newline, K::Dedent, K::Eof};
    CHECK(kinds == want);
    CHECK(errors == 0);
}

TEST(lexer_newlines_ignored_inside_brackets) {
    auto kinds = lex_kinds("f(1,\n        2)\n");
    std::vector<K> want = {K::Identifier, K::LParen, K::IntLiteral, K::Comma, K::IntLiteral,
                           K::RParen, K::Newline, K::Eof};
    CHECK(kinds == want);
}

TEST(lexer_numbers_and_ranges) {
    size_t errors = 99;
    auto kinds = lex_kinds("0..10 0..=3 1_000 0xFF 0b1010 3.14 1e-9 2.5E+3", &errors);
    std::vector<K> want = {K::IntLiteral, K::DotDot, K::IntLiteral, K::IntLiteral, K::DotDotEq,
                           K::IntLiteral, K::IntLiteral, K::IntLiteral, K::IntLiteral,
                           K::FloatLiteral, K::FloatLiteral, K::FloatLiteral, K::Newline, K::Eof};
    CHECK(kinds == want);
    CHECK(errors == 0);
    CHECK(lex_errors("12abc") == 1);
    CHECK(lex_errors("0x") == 1);
}

TEST(lexer_operators) {
    auto kinds = lex_kinds("// //= ** **x -> != <= << >>= .");
    std::vector<K> want = {K::SlashSlash, K::SlashSlashEq, K::StarStar, K::StarStar, K::Identifier,
                           K::Arrow, K::NotEq, K::LtEq, K::Shl, K::Shr, K::Eq, K::Dot,
                           K::Newline, K::Eof};
    CHECK(kinds == want);
    CHECK(lex_errors("!x") == 1);
}

TEST(lexer_strings) {
    size_t errors = 99;
    auto kinds = lex_kinds(R"(print("x = {d["k"]} {{literal}} \{ {x:>{w}}") r"C:\path")", &errors);
    std::vector<K> want = {K::Identifier, K::LParen, K::StringLiteral, K::RParen, K::StringLiteral,
                           K::Newline, K::Eof};
    CHECK(kinds == want);
    CHECK(errors == 0);
    CHECK(lex_errors("\"no end\n") == 1);
    CHECK(lex_errors("\"bad \\q escape\"") == 1);  // not a raw string: CHECK's #cond would re-escape it
    CHECK(lex_errors(R"("open {x")") == 1);
    CHECK(lex_errors(R"("single } brace")") == 1);
}

TEST(lexer_chars) {
    CHECK(lex_kinds(R"('a' '\n')") == (std::vector<K>{K::CharLiteral, K::CharLiteral, K::Newline, K::Eof}));
    CHECK(lex_errors("''") == 1);
    CHECK(lex_errors("'ab'") >= 1);
    CHECK(lex_errors("'\\") == 1);  // backslash at end of file
}

TEST(lexer_indentation_errors) {
    CHECK(lex_errors("a:\n\tb\n") == 1);              // tab
    CHECK(lex_errors("a:\n   b\n") == 1);             // 3 spaces
    CHECK(lex_errors("a:\n        b\n") == 1);        // too deep
    CHECK(lex_errors("a:\n    b:\n        c\n  d\n") >= 1);  // dedent to no level
}

TEST(lexer_bracket_errors) {
    CHECK(lex_errors("f(1\n") == 1);
    CHECK(lex_errors("x)\n") == 1);
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
