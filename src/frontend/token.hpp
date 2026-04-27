#pragma once

#include <string_view>

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
        Eof
    };

    // Type   | Location | Lexeme
    // Struct | 10:1     | struct
    struct Token
    {
        u32 line {};
        // u16 should be enough since when would a source file have a line so big it cant fit in a u16
        u16 column {};
        TokenType type {};
        std::string_view lexeme;
    };

    std::string to_string(Token token);

    bool is_binary_op(TokenType type);
}
