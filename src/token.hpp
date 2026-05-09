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
        // binary ops start
        _BinaryStart,
        Minus,
        Plus,
        ForwardSlash,
        Star,
        Percent,
        BangEqual,
        Equal,
        EqualEqual,
        At,
        Caret,
        Tilde,
        Greater,
        GreaterEqual,
        Less,
        LessEqual,
        _BinaryEnd,
        // binary ops end
        PlusEqual,
        MinusEqual,
        StarEqual,
        SlashEqual,
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
        Bang,
        Arrow,
        And,
        Or,
        BitwiseOr,
        BitwiseAnd,
        Fn,
        Do,
        True,
        Else,
        False,
        Return,
        Import,
        Private,
        Const,
        Inout,
        Var,
        Extern,
        AlignOf,
        TypeOf,
        Alias,
        Enum,
        Error,
        Async,
        Await,
        Static,
        For,
        In,
        If,
        While,
        Match,
        Continue,
        Break,
        Default,
        Identifier,
        StringLiteral,
        CharLiteral,
        IntegerLiteral,
        UIntegerLiteral,
        LIntegerLiteral,
        ULIntegerLiteral,
        DecimalLiteral,
        LDecimalLiteral,
        Signed,
        Sizeof,
        Struct,
        Trait,
        Union,
        // NOTE: temporary so i can debug the compiler
        // will replace later
        Println,
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
