#pragma once

#include <string_view>

#include "location.hpp"

namespace pars
{
    enum class TokenType : u8
    {
        TokenError,
        Colon,
        Question,

        _ComparisonStart,

        Greater,
        GreaterEqual,
        Less,
        LessEqual,
        And,
        Or,
        BangEqual,
        EqualEqual,

        _ComparisonEnd,

        _BinaryStart,

        Minus,
        Plus,
        ForwardSlash,
        Star,
        StarStar,
        Percent,
        DotDot,
        DotDotEqual,

        _BinaryEnd,

        _UnaryStart,
        Tilde,
        Caret,
        At,
        Bang,
        _UnaryEnd,

        _AssignmentStart,

        PlusEqual,
        MinusEqual,
        StarEqual,
        SlashEqual,
        Equal,

        _AssignmentEnd,

        PlusPlus,
        MinusMinus,
        LeftParen,
        RightParen,
        LeftBrace,
        RightBrace,
        LeftBracket,
        RightBracket,
        Comma,
        Dollar,
        Hash,
        Dot,
        SemiColon,
        Ampersand,
        Pipe,
        Arrow, // =>
        Fn,
        True,
        Else,
        False,
        Return,
        Import,
        // marks tokens that can be used for attributes and thus will generate an keyword expression
        _AttributeKeywordStart,
        Extern,
        Private,
        Static,
        Volatile,
        _AttributeKeywordEnd,
        Const,
        Inout,
        Var,
        AlignOf,
        TypeOf,
        Alias,
        Distinct,
        Enum,
        Error,
        Async,
        Await,
        For,
        In,
        If,
        Loop,
        While,
        Match,
        Continue,
        Break,
        Cast,
        Nil,
        Identifier,
        StringLiteral,
        CharLiteral,
        IntegerLiteral,
        UIntegerLiteral,
        LIntegerLiteral,
        ULIntegerLiteral,
        DecimalLiteral,
        LDecimalLiteral,
        Sizeof,
        Struct,
        Trait,
        Union,
        Eof
    };

    struct Token
    {
        Location location;
        TokenType type;
        std::string_view lexeme;
    };

    bool is_binary_op(TokenType type);

    std::string report_token(Token token, std::string_view message);
}
