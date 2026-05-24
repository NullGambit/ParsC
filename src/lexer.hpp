#pragma once
#include <array>
#include <optional>
#include <string_view>

#include "file_manager.hpp"
#include "text_reader.hpp"
#include "token.hpp"

namespace pars
{
    enum class TokenType : u8;

    enum class OpType
    {
        _ComparisonStart,
        Equal,
        UnEqual,
        Less,
        Greater,
        LessEqual,
        GreaterEqual,
        _ComparisonEnd,
        _BinaryStart,
        Add,
        Sub,
        Mul,
        Div,
        _BinaryEnd,
    };

    class Lexer
    {
    public:

        Lexer() = default;
        Lexer(SourceFile source);

        void set_source(SourceFile source);

        Token advance();

        bool match(TokenType type);
        bool match_next(TokenType type);
        Token expect(TokenType type);
        Token peek();
        bool peek(TokenType type);
        Token peek_next();
        bool peek_next(TokenType type);
        Token peek_last();
        bool peek_last(TokenType type);

        [[nodiscard]]
        bool has_next() const;

    private:
        u16 m_file_id;
        TextReader m_reader;
        std::optional<Token> m_last_token;
        std::optional<Token> m_current_token;
        std::optional<Token> m_next_token;

        Token build_token(TokenType type, std::string_view lexeme_override = {});
        Token build_token(char match, TokenType tk1, TokenType tk2);
        Token build_error(std::string_view message);

        Token build_digit();
        Token build_string();
        Token build_char();
        // build identifier or keyword
        Token build_identifier();

        Token advance_one();
    };
}
