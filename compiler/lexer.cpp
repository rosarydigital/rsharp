#include "lexer.h"

namespace rsharp {

namespace {

bool is_digit(char c) { return c >= '0' && c <= '9'; }
bool is_ident_start(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
bool is_ident_char(char c) { return is_ident_start(c) || is_digit(c); }
bool is_hex(char c) { return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }

// How many bytes the UTF-8 character starting with byte `c` occupies.
size_t utf8_len(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c >> 5) == 0x6) return 2;
    if ((c >> 4) == 0xE) return 3;
    if ((c >> 3) == 0x1E) return 4;
    return 1;
}

bool valid_digit(char base, char c) {
    switch (base) {
        case 'x': return is_hex(c);
        case 'b': return c == '0' || c == '1';
        case 'o': return c >= '0' && c <= '7';
        default: return is_digit(c);
    }
}

}  // namespace

Lexer::Lexer(const SourceFile& file, DiagnosticSink& diags)
    : diags_(diags), text_(file.text()) {}

// ---------------------------------------------------------------- helpers

char Lexer::peek(size_t ahead) const {
    return pos_ + ahead < text_.size() ? text_[pos_ + ahead] : '\0';
}

bool Lexer::starts_with(std::string_view s) const {
    return text_.substr(pos_, s.size()) == s;
}

Span Lexer::span_at(size_t start, size_t end) const {
    Span s;
    s.offset = static_cast<uint32_t>(start);
    s.length = static_cast<uint32_t>(end > start ? end - start : 1);
    s.line = line_;
    s.column = static_cast<uint32_t>(start >= line_start_ ? start - line_start_ + 1 : 1);
    return s;
}

void Lexer::error(size_t start, size_t end, std::string message, std::string help) {
    diags_.error(span_at(start, end), std::move(message), std::move(help));
}

void Lexer::emit(TokenKind kind, size_t start, size_t end) {
    Token t;
    t.kind = kind;
    t.span = span_at(start, end);
    t.span.length = static_cast<uint32_t>(end - start);  // allow zero-length tokens
    t.text = text_.substr(start, end - start);
    out_.push_back(t);
}

void Lexer::newline() {
    ++pos_;
    ++line_;
    line_start_ = pos_;
}

// ---------------------------------------------------------------- main loop

std::vector<Token> Lexer::tokenize() {
    const size_t n = text_.size();
    bool at_line_start = true;

    while (pos_ < n) {
        if (at_line_start && paren_depth_ == 0) {
            if (!lex_indentation()) continue;  // blank line, stay at line start
            at_line_start = false;
            if (pos_ >= n) break;
        }

        char c = text_[pos_];

        if (c == ' ' || c == '\t' || c == '\r') {
            ++pos_;
            continue;
        }
        if (c == '#') {  // comment runs to end of line
            while (pos_ < n && text_[pos_] != '\n') ++pos_;
            continue;
        }
        if (c == '\n') {
            if (paren_depth_ == 0) {
                if (!out_.empty() && out_.back().kind != TokenKind::Newline) {
                    emit(TokenKind::Newline, pos_, pos_ + 1);
                }
                at_line_start = true;
            }
            newline();
            continue;
        }

        if (is_digit(c)) lex_number();
        else if (is_ident_start(c)) lex_identifier();
        else if (c == '"') lex_string(pos_, false);
        else if (c == '\'') lex_char();
        else lex_operator();
    }

    if (paren_depth_ > 0) {
        error(n > 0 ? n - 1 : 0, n, "unclosed '(', '[' or '{'", "add the matching closing bracket");
    }

    // Terminate the last logical line, then close any open blocks.
    if (!out_.empty() && out_.back().kind != TokenKind::Newline) emit(TokenKind::Newline, n, n);
    while (indents_.size() > 1) {
        indents_.pop_back();
        emit(TokenKind::Dedent, n, n);
    }
    emit(TokenKind::Eof, n, n);
    return std::move(out_);
}

// ---------------------------------------------------------------- indentation

bool Lexer::lex_indentation() {
    const size_t n = text_.size();
    const size_t start = pos_;
    size_t p = pos_;
    uint32_t width = 0;
    bool tab_reported = false;

    while (p < n && (text_[p] == ' ' || text_[p] == '\t')) {
        if (text_[p] == '\t' && !tab_reported) {
            error(p, p + 1, "tabs are not allowed for indentation", "use 4 spaces per indentation level");
            tab_reported = true;
        }
        width += text_[p] == '\t' ? 4 : 1;
        ++p;
    }

    // Blank line or comment-only line: no tokens, doesn't affect indentation.
    if (p >= n || text_[p] == '\n' || text_[p] == '\r' || text_[p] == '#') {
        while (p < n && text_[p] != '\n') ++p;
        pos_ = p;
        if (pos_ < n) newline();
        return false;
    }

    pos_ = p;
    const uint32_t current = indents_.back();

    if (width % 4 != 0 && !tab_reported) {
        error(start, p, "indentation must be a multiple of 4 spaces", "use 4 spaces per indentation level");
    }

    if (width > current) {
        if (width % 4 == 0 && width != current + 4) {
            error(start, p, "indentation is too deep", "indent exactly 4 spaces per level");
        }
        indents_.push_back(width);
        emit(TokenKind::Indent, start, p);
    } else if (width < current) {
        while (indents_.back() > width) {
            indents_.pop_back();
            emit(TokenKind::Dedent, start, p);
        }
        if (indents_.back() != width) {
            error(start, p, "unindent does not match any outer indentation level",
                  "line up with an enclosing block's indentation");
            indents_.push_back(width);  // recover
        }
    }
    return true;
}

// ---------------------------------------------------------------- numbers

void Lexer::lex_number() {
    const size_t n = text_.size();
    const size_t start = pos_;
    bool is_float = false;

    auto lower = [](char c) { return static_cast<char>(c | 0x20); };

    if (text_[pos_] == '0' && (lower(peek(1)) == 'x' || lower(peek(1)) == 'b' || lower(peek(1)) == 'o')) {
        char base = lower(peek(1));
        pos_ += 2;
        size_t digits_start = pos_;
        while (pos_ < n && (valid_digit(base, text_[pos_]) || text_[pos_] == '_')) ++pos_;
        if (pos_ == digits_start) {
            error(start, pos_, "expected digits after the base prefix");
        }
    } else {
        while (pos_ < n && (is_digit(text_[pos_]) || text_[pos_] == '_')) ++pos_;
        // A '.' only starts a fraction if a digit follows, so `0..10` is a range.
        if (peek() == '.' && is_digit(peek(1))) {
            is_float = true;
            ++pos_;
            while (pos_ < n && (is_digit(text_[pos_]) || text_[pos_] == '_')) ++pos_;
        }
        char e = peek();
        if ((e == 'e' || e == 'E') &&
            (is_digit(peek(1)) || ((peek(1) == '+' || peek(1) == '-') && is_digit(peek(2))))) {
            is_float = true;
            ++pos_;
            if (peek() == '+' || peek() == '-') ++pos_;
            while (pos_ < n && is_digit(text_[pos_])) ++pos_;
        }
    }

    // `12abc` or `0b102` : letters/digits glued onto a number
    if (pos_ < n && is_ident_char(text_[pos_])) {
        size_t bad = pos_;
        while (pos_ < n && is_ident_char(text_[pos_])) ++pos_;
        error(bad, pos_, "invalid character in number literal", "numbers can't be followed by letters; use `as` to convert types");
    }

    emit(is_float ? TokenKind::FloatLiteral : TokenKind::IntLiteral, start, pos_);
}

// ---------------------------------------------------------------- identifiers

void Lexer::lex_identifier() {
    const size_t n = text_.size();
    const size_t start = pos_;
    while (pos_ < n && is_ident_char(text_[pos_])) ++pos_;
    std::string_view word = text_.substr(start, pos_ - start);

    // r"raw string"
    if (word == "r" && peek() == '"') {
        lex_string(start, true);
        return;
    }
    emit(keyword_or_identifier(word), start, pos_);
}

// ---------------------------------------------------------------- strings

// Scans a `{ ... }` interpolation. pos_ is at the '{'. Handles nested braces
// and nested string literals, e.g. "{d["key"]}" or "{x:>{width}}".
bool Lexer::skip_interpolation() {
    const size_t n = text_.size();
    const size_t open = pos_;
    int depth = 1;
    ++pos_;
    while (depth > 0) {
        if (pos_ >= n || text_[pos_] == '\n') {
            error(open, open + 1, "unterminated '{' in string interpolation", "add a closing '}'");
            return false;
        }
        char c = text_[pos_];
        if (c == '"') {  // nested string literal
            ++pos_;
            while (pos_ < n && text_[pos_] != '"' && text_[pos_] != '\n') {
                if (text_[pos_] == '\\' && pos_ + 1 < n) ++pos_;
                ++pos_;
            }
            if (pos_ < n && text_[pos_] == '"') {
                ++pos_;
            } else {
                error(open, open + 1, "unterminated string inside interpolation");
                return false;
            }
            continue;
        }
        if (c == '{') ++depth;
        else if (c == '}') --depth;
        ++pos_;
    }
    return true;
}

void Lexer::lex_string(size_t start, bool raw) {
    const size_t n = text_.size();
    ++pos_;  // opening quote
    bool ok = true;

    while (true) {
        if (pos_ >= n || text_[pos_] == '\n' || (text_[pos_] == '\r' && peek(1) == '\n')) {
            error(start, start + 1, "unterminated string literal", "strings must end with a closing '\"' on the same line");
            emit(TokenKind::Invalid, start, pos_);
            return;
        }
        char c = text_[pos_];
        if (c == '"') {
            ++pos_;
            break;
        }
        if (raw) {
            ++pos_;
            continue;
        }
        if (c == '\\') {
            char e = peek(1);
            switch (e) {
                case 'n': case 't': case 'r': case '0':
                case '"': case '\'': case '\\': case '{': case '}':
                    pos_ += 2;
                    break;
                default:
                    error(pos_, pos_ + 2, "unknown escape sequence",
                          "valid escapes: \\n \\t \\r \\0 \\\" \\' \\\\ \\{ \\}");
                    ok = false;
                    pos_ += (e != '\0' && e != '\n') ? 2 : 1;
            }
            continue;
        }
        if (c == '{') {
            if (peek(1) == '{') {  // "{{" is a literal brace
                pos_ += 2;
                continue;
            }
            if (!skip_interpolation()) {
                emit(TokenKind::Invalid, start, pos_);
                return;
            }
            continue;
        }
        if (c == '}') {
            if (peek(1) == '}') {
                pos_ += 2;
                continue;
            }
            error(pos_, pos_ + 1, "single '}' in string", "write '}}' or '\\}' for a literal brace");
            ok = false;
            ++pos_;
            continue;
        }
        ++pos_;
    }
    emit(ok ? TokenKind::StringLiteral : TokenKind::Invalid, start, pos_);
}

void Lexer::lex_char() {
    const size_t n = text_.size();
    const size_t start = pos_;
    ++pos_;  // opening quote

    if (pos_ >= n || text_[pos_] == '\n') {
        error(start, start + 1, "unterminated character literal");
        emit(TokenKind::Invalid, start, pos_);
        return;
    }
    if (text_[pos_] == '\'') {
        ++pos_;
        error(start, pos_, "empty character literal", "a char holds exactly one character, like 'a'");
        emit(TokenKind::Invalid, start, pos_);
        return;
    }
    if (text_[pos_] == '\\') pos_ += 2;
    else pos_ += utf8_len(static_cast<unsigned char>(text_[pos_]));
    if (pos_ > n) pos_ = n;  // escape or multi-byte char cut off by end of file

    if (pos_ < n && text_[pos_] == '\'') {
        ++pos_;
        emit(TokenKind::CharLiteral, start, pos_);
    } else {
        error(start, pos_, "unterminated character literal", "a char holds exactly one character; use \"...\" for strings");
        emit(TokenKind::Invalid, start, pos_);
    }
}

// ---------------------------------------------------------------- operators

void Lexer::lex_operator() {
    const size_t start = pos_;
    char c = text_[pos_];

    auto take = [&](TokenKind k, size_t len) {
        pos_ += len;
        emit(k, start, pos_);
    };

    switch (c) {
        case '(': ++paren_depth_; return take(TokenKind::LParen, 1);
        case '[': ++paren_depth_; return take(TokenKind::LBracket, 1);
        case '{': ++paren_depth_; return take(TokenKind::LBrace, 1);
        case ')': case ']': case '}': {
            if (paren_depth_ > 0) --paren_depth_;
            else error(start, start + 1, std::string("unmatched '") + c + "'", "there is no matching opening bracket");
            return take(c == ')' ? TokenKind::RParen : c == ']' ? TokenKind::RBracket : TokenKind::RBrace, 1);
        }
        case ',': return take(TokenKind::Comma, 1);
        case ':': return take(TokenKind::Colon, 1);
        case '?': return take(TokenKind::Question, 1);
        case '~': return take(TokenKind::Tilde, 1);
        case '^': return take(TokenKind::Caret, 1);
        case '&': return take(TokenKind::Amp, 1);
        case '|': return take(TokenKind::Pipe, 1);
        case '.':
            if (starts_with("..=")) return take(TokenKind::DotDotEq, 3);
            if (starts_with("..")) return take(TokenKind::DotDot, 2);
            return take(TokenKind::Dot, 1);
        case '+':
            if (starts_with("+=")) return take(TokenKind::PlusEq, 2);
            return take(TokenKind::Plus, 1);
        case '-':
            if (starts_with("->")) return take(TokenKind::Arrow, 2);
            if (starts_with("-=")) return take(TokenKind::MinusEq, 2);
            return take(TokenKind::Minus, 1);
        case '*':
            if (starts_with("**")) return take(TokenKind::StarStar, 2);
            if (starts_with("*=")) return take(TokenKind::StarEq, 2);
            return take(TokenKind::Star, 1);
        case '/':
            if (starts_with("//=")) return take(TokenKind::SlashSlashEq, 3);
            if (starts_with("//")) return take(TokenKind::SlashSlash, 2);
            if (starts_with("/=")) return take(TokenKind::SlashEq, 2);
            return take(TokenKind::Slash, 1);
        case '%':
            if (starts_with("%=")) return take(TokenKind::PercentEq, 2);
            return take(TokenKind::Percent, 1);
        case '=':
            if (starts_with("==")) return take(TokenKind::EqEq, 2);
            return take(TokenKind::Eq, 1);
        case '!':
            if (starts_with("!=")) return take(TokenKind::NotEq, 2);
            break;  // a lone '!' is invalid; R# uses `not`
        case '<':
            if (starts_with("<<")) return take(TokenKind::Shl, 2);
            if (starts_with("<=")) return take(TokenKind::LtEq, 2);
            return take(TokenKind::Lt, 1);
        case '>':
            if (starts_with(">>")) return take(TokenKind::Shr, 2);
            if (starts_with(">=")) return take(TokenKind::GtEq, 2);
            return take(TokenKind::Gt, 1);
        default:
            break;
    }

    // Anything else is an invalid character. Consume a whole UTF-8 character.
    size_t len = utf8_len(static_cast<unsigned char>(c));
    pos_ += len;
    std::string shown(text_.substr(start, len));
    if (c == '!') {
        error(start, start + 1, "unexpected '!'", "use `not` for logical negation and `!=` for inequality");
    } else {
        error(start, pos_, "unexpected character '" + shown + "'");
    }
    emit(TokenKind::Invalid, start, pos_);
}

}  // namespace rsharp
