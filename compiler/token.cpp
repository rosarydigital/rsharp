#include "token.h"

#include <string>
#include <unordered_map>

namespace rsharp {

const char* token_kind_name(TokenKind k) {
    switch (k) {
        case TokenKind::Newline: return "Newline";
        case TokenKind::Indent: return "Indent";
        case TokenKind::Dedent: return "Dedent";
        case TokenKind::Eof: return "Eof";
        case TokenKind::Identifier: return "Identifier";
        case TokenKind::IntLiteral: return "Int";
        case TokenKind::FloatLiteral: return "Float";
        case TokenKind::CharLiteral: return "Char";
        case TokenKind::StringLiteral: return "String";

        case TokenKind::KwFn: return "'fn'";
        case TokenKind::KwLet: return "'let'";
        case TokenKind::KwVar: return "'var'";
        case TokenKind::KwIf: return "'if'";
        case TokenKind::KwElif: return "'elif'";
        case TokenKind::KwElse: return "'else'";
        case TokenKind::KwWhile: return "'while'";
        case TokenKind::KwFor: return "'for'";
        case TokenKind::KwIn: return "'in'";
        case TokenKind::KwReturn: return "'return'";
        case TokenKind::KwBreak: return "'break'";
        case TokenKind::KwContinue: return "'continue'";
        case TokenKind::KwStruct: return "'struct'";
        case TokenKind::KwEnum: return "'enum'";
        case TokenKind::KwImpl: return "'impl'";
        case TokenKind::KwMatch: return "'match'";
        case TokenKind::KwImport: return "'import'";
        case TokenKind::KwFrom: return "'from'";
        case TokenKind::KwExtern: return "'extern'";
        case TokenKind::KwPub: return "'pub'";
        case TokenKind::KwAnd: return "'and'";
        case TokenKind::KwOr: return "'or'";
        case TokenKind::KwNot: return "'not'";
        case TokenKind::KwTrue: return "'true'";
        case TokenKind::KwFalse: return "'false'";
        case TokenKind::KwNone: return "'none'";
        case TokenKind::KwOk: return "'ok'";
        case TokenKind::KwErr: return "'err'";
        case TokenKind::KwSome: return "'some'";
        case TokenKind::KwUnsafe: return "'unsafe'";
        case TokenKind::KwAs: return "'as'";
        case TokenKind::KwMove: return "'move'";

        case TokenKind::LParen: return "'('";
        case TokenKind::RParen: return "')'";
        case TokenKind::LBracket: return "'['";
        case TokenKind::RBracket: return "']'";
        case TokenKind::LBrace: return "'{'";
        case TokenKind::RBrace: return "'}'";
        case TokenKind::Comma: return "','";
        case TokenKind::Colon: return "':'";
        case TokenKind::Dot: return "'.'";
        case TokenKind::DotDot: return "'..'";
        case TokenKind::DotDotEq: return "'..='";
        case TokenKind::Arrow: return "'->'";
        case TokenKind::Question: return "'?'";

        case TokenKind::Plus: return "'+'";
        case TokenKind::Minus: return "'-'";
        case TokenKind::Star: return "'*'";
        case TokenKind::Slash: return "'/'";
        case TokenKind::SlashSlash: return "'//'";
        case TokenKind::Percent: return "'%'";
        case TokenKind::StarStar: return "'**'";
        case TokenKind::Amp: return "'&'";
        case TokenKind::Pipe: return "'|'";
        case TokenKind::Caret: return "'^'";
        case TokenKind::Tilde: return "'~'";
        case TokenKind::Shl: return "'<<'";
        case TokenKind::Shr: return "'>>'";
        case TokenKind::Eq: return "'='";
        case TokenKind::EqEq: return "'=='";
        case TokenKind::NotEq: return "'!='";
        case TokenKind::Lt: return "'<'";
        case TokenKind::LtEq: return "'<='";
        case TokenKind::Gt: return "'>'";
        case TokenKind::GtEq: return "'>='";
        case TokenKind::PlusEq: return "'+='";
        case TokenKind::MinusEq: return "'-='";
        case TokenKind::StarEq: return "'*='";
        case TokenKind::SlashEq: return "'/='";
        case TokenKind::SlashSlashEq: return "'//='";
        case TokenKind::PercentEq: return "'%='";

        case TokenKind::Invalid: return "Invalid";
    }
    return "?";
}

TokenKind keyword_or_identifier(std::string_view w) {
    static const std::unordered_map<std::string, TokenKind> kw = {
        {"fn", TokenKind::KwFn},         {"let", TokenKind::KwLet},
        {"var", TokenKind::KwVar},       {"if", TokenKind::KwIf},
        {"elif", TokenKind::KwElif},     {"else", TokenKind::KwElse},
        {"while", TokenKind::KwWhile},   {"for", TokenKind::KwFor},
        {"in", TokenKind::KwIn},         {"return", TokenKind::KwReturn},
        {"break", TokenKind::KwBreak},   {"continue", TokenKind::KwContinue},
        {"struct", TokenKind::KwStruct}, {"enum", TokenKind::KwEnum},
        {"impl", TokenKind::KwImpl},     {"match", TokenKind::KwMatch},
        {"import", TokenKind::KwImport}, {"from", TokenKind::KwFrom},
        {"extern", TokenKind::KwExtern}, {"pub", TokenKind::KwPub},
        {"and", TokenKind::KwAnd},       {"or", TokenKind::KwOr},
        {"not", TokenKind::KwNot},       {"true", TokenKind::KwTrue},
        {"false", TokenKind::KwFalse},   {"none", TokenKind::KwNone},
        {"ok", TokenKind::KwOk},         {"err", TokenKind::KwErr},
        {"some", TokenKind::KwSome},     {"unsafe", TokenKind::KwUnsafe},
        {"as", TokenKind::KwAs},         {"move", TokenKind::KwMove},
    };
    auto it = kw.find(std::string(w));
    return it == kw.end() ? TokenKind::Identifier : it->second;
}

}  // namespace rsharp
