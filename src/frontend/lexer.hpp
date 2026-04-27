#pragma once
#include <string_view>

#include "text_reader.hpp"

namespace pars
{
    struct Token;
    enum class TokenType : u8;

    class Lexer
    {
    public:
        Lexer(std::string_view source);

        void set_source(std::string_view source);

        Token advance();

        [[nodiscard]]
        bool has_next() const;

    private:
        u32 m_offset;
        u32 m_current;
        TextReader m_reader;

        Token build_token(TokenType type, std::string_view lexeme_override = {});
        Token build_token(char match, TokenType tk1, TokenType tk2);
        Token build_error(std::string_view message);

        Token build_digit();
        Token build_string();
        Token build_char();
        // build identifier or keyword
        Token build_identifier();
    };
}
