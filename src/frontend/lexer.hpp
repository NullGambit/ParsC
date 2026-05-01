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

    class Lexer
    {
    public:

        Lexer() = default;
        Lexer(SourceFile source);

        void set_source(SourceFile source);

        Token advance_one();

        // advances by 3 tokens. will store last, current, next tokens
        Token advance();

        bool match(TokenType type);
        bool match_next(TokenType type);
        Token expect(TokenType type);
        Token peak();
        bool peak(TokenType type);
        Token peak_next();
        bool peak_next(TokenType type);
        Token peak_last();
        bool peak_last(TokenType type);

        [[nodiscard]]
        bool has_next() const;

    private:
        SourceFile m_src;
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
    };
}
