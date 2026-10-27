#pragma once
// Token kinds for R#, matching section 2 of the language spec.

#include <string_view>

#include "source.h"

namespace rsharp {

enum class TokenKind {
    // structure (produced by the indent-aware lexer)
    Newline,
    Indent,
    Dedent,
    Eof,

    // literals and names
    Identifier,
    IntLiteral,
    FloatLiteral,
    CharLiteral,
    StringLiteral,  // includes interpolated strings; parts are split later

    // keywords
    KwFn, KwLet, KwVar, KwIf, KwElif, KwElse, KwWhile, KwFor, KwIn,
    KwReturn, KwBreak, KwContinue, KwStruct, KwEnum, KwImpl, KwMatch,
    KwImport, KwFrom, KwExtern, KwPub, KwAnd, KwOr, KwNot,
    KwTrue, KwFalse, KwNone, KwOk, KwErr, KwSome, KwUnsafe, KwAs, KwMove,

    // punctuation
    LParen, RParen, LBracket, RBracket, LBrace, RBrace,
    Comma, Colon, Dot, DotDot, DotDotEq, Arrow, Question,

    // operators
    Plus, Minus, Star, Slash, SlashSlash, Percent, StarStar,
    Amp, Pipe, Caret, Tilde, Shl, Shr,
    Eq, EqEq, NotEq, Lt, LtEq, Gt, GtEq,
    PlusEq, MinusEq, StarEq, SlashEq, SlashSlashEq, PercentEq,

    // lexer error placeholder
    Invalid,
};

struct Token {
    TokenKind kind = TokenKind::Invalid;
    Span span;
    std::string_view text;  // view into the SourceFile text
};

// Human-readable name of a token kind, for error messages ("expected ':'").
const char* token_kind_name(TokenKind kind);

// Returns the keyword kind for an identifier, or TokenKind::Identifier.
TokenKind keyword_or_identifier(std::string_view word);

}  // namespace rsharp
