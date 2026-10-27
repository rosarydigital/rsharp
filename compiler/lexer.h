#pragma once
// The R# lexer (spec section 2).
//
// Turns source text into tokens, including the structural tokens that make
// indentation-based blocks easy to parse:
//
//   Newline  end of a logical line
//   Indent   the next line is exactly one level (4 spaces) deeper
//   Dedent   one block closed (several in a row if several close at once)
//
// Rules implemented here:
//   * indentation is 4 spaces per level; tabs are an error
//   * blank lines and comment-only lines are ignored
//   * newlines and indentation are ignored inside ( ), [ ], { }
//   * string literals are lexed as ONE token, including any {interpolation};
//     splitting a string into text/expression parts happens in the parser
//   * lexing never stops at an error: problems are reported to the
//     DiagnosticSink and lexing continues so users see many errors at once

#include <string>
#include <vector>

#include "diagnostics.h"
#include "source.h"
#include "token.h"

namespace rsharp {

class Lexer {
public:
    Lexer(const SourceFile& file, DiagnosticSink& diags);

    // Lex the whole file. The last token is always Eof.
    // Token::text views point into the SourceFile, which must outlive them.
    std::vector<Token> tokenize();

private:
    // helpers
    char peek(size_t ahead = 0) const;
    bool starts_with(std::string_view s) const;
    Span span_at(size_t start, size_t end) const;
    void error(size_t start, size_t end, std::string message, std::string help = {});
    void emit(TokenKind kind, size_t start, size_t end);
    void newline();  // consume one '\n' and update line tracking

    // structure
    bool lex_indentation();  // false if the line was blank and fully consumed

    // token kinds
    void lex_number();
    void lex_identifier();
    void lex_string(size_t start, bool raw);
    bool skip_interpolation();
    void lex_char();
    void lex_operator();

    DiagnosticSink& diags_;
    std::string_view text_;
    size_t pos_ = 0;
    uint32_t line_ = 1;
    size_t line_start_ = 0;
    int paren_depth_ = 0;
    std::vector<uint32_t> indents_{0};
    std::vector<Token> out_;
};

}  // namespace rsharp
