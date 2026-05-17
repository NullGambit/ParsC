#include "lexer.hpp"

#include <locale>

#include "frontend_error.hpp"
#include "token.hpp"
#include "containers/hash_map.hpp"
#include "magic_enum/magic_enum.hpp"
#include "util/fmt.hpp"

namespace pars
{
    // TODO: replace with a perfect hash table
    static HashMap<std::string_view, TokenType> g_keywords
    {
        {"struct", TokenType::Struct},
        {"trait", TokenType::Trait},
        {"union", TokenType::Union},
        {"static", TokenType::Static},
        {"async", TokenType::Async},
        {"await", TokenType::Await},
        {"enum", TokenType::Enum},
        {"do", TokenType::Do},
        {"true", TokenType::True},
        {"false", TokenType::False},
        {"if", TokenType::If},
        {"else", TokenType::Else},
        {"return", TokenType::Return},
        {"const", TokenType::Const},
        {"import", TokenType::Import},
        {"private", TokenType::Private},
        {"inout", TokenType::Inout},
        {"var", TokenType::Var},
        {"extern", TokenType::Extern},
        {"alignof", TokenType::AlignOf},
        {"alias", TokenType::Alias},
        {"distinct", TokenType::Distinct},
        {"for", TokenType::For},
        {"while", TokenType::While},
        {"match", TokenType::Match},
        {"continue", TokenType::Continue},
        {"break", TokenType::Break},
        {"default", TokenType::Default},
        {"signed", TokenType::Signed},
        {"sizeof", TokenType::Sizeof},
        {"error", TokenType::Error},
        {"in", TokenType::In},
        {"and", TokenType::And},
        {"or", TokenType::Or},
        {"fn", TokenType::Fn},
    };
}

pars::Lexer::Lexer(SourceFile source)
{
    set_source(source);
}

void pars::Lexer::set_source(SourceFile source)
{
    m_file_id = source.id;
    m_reader.set_source(source.contents);
}

pars::Token pars::Lexer::advance_one()
{
    m_reader.skip_insignificant();

    if (m_reader.peek() == '/' && m_reader.peek_next() == '/')
    {
        m_reader.skip_until('\n');
        m_reader.skip_insignificant();
    }

    const auto c = m_reader.advance();

    using enum TokenType;

    switch (c)
    {
        case '+':
        {
            if (m_reader.match('+'))
            {
                return build_token(PlusEqual);
            }

            return build_token('=', PlusEqual, Plus);
        }
        case '-':
        {
            if (m_reader.match('-'))
            {
                return build_token(MinusMinus);
            }

            return build_token('=', MinusEqual, Minus);
        }
        case '=':
        {
            if (m_reader.match('='))
            {
                return build_token(EqualEqual);
            }
            if (m_reader.match('>'))
            {
                return build_token(Arrow);
            }

            return build_token(Equal);
        }
        case '*': return build_token('=', StarEqual, Star);
        case '/': return build_token('=', SlashEqual, ForwardSlash);
        case '<': return build_token('=', LessEqual, Less);
        case '>': return build_token('=', GreaterEqual, Greater);
        case '!': return build_token('=', BangEqual, Bang);
        case '%': return build_token(Percent);
        case '$': return build_token(Dollar);
        case '&': return build_token(BitwiseAnd);
        case '|': return build_token(BitwiseOr);
        case '(': return build_token(LeftParen);
        case ')': return build_token(RightParen);
        case '{': return build_token(LeftBrace);
        case '}': return build_token(RightBrace);
        case '#': return build_token(Hash);
        case '@': return build_token(At);
        case '~': return build_token(Tilde);
        case ',': return build_token(Comma);
        case '.': return build_token(Dot);
        case ';': return build_token(SemiColon);
        case ':': return build_token(Colon);
        case '?': return build_token(Question);
        case '[': return build_token(LeftBracket);
        case ']': return build_token(RightBracket);
        case '"': return build_string();
        case '\'': return build_char();
        case '\0': return build_token(Eof);
        default:
        {
            if (std::isdigit(c))
            {
                return build_digit();
            }
            if (TextReader::is_identifier(c))
            {
                return build_identifier();
            }

            throw FrontendError(build_error(""), "unexpected token found");
        }
    }
}

pars::Token pars::Lexer::advance()
{
    if (!has_next())
    {
        m_current_token = build_token(TokenType::Eof);
        return m_current_token.value();
    }

    m_last_token = m_current_token;

    if (m_next_token.has_value())
    {
        m_current_token = m_next_token;
        m_next_token = std::nullopt;
    }
    else
    {
        m_current_token = advance_one();
    }

    return m_current_token.value();
}

bool pars::Lexer::match(TokenType type)
{
    if (peek(type))
    {
        advance();
        return true;
    }

    return false;
}

bool pars::Lexer::match_next(TokenType type)
{
    if (peek_next(type))
    {
        advance();
        return true;
    }

    return false;
}

pars::Token pars::Lexer::expect(TokenType type)
{
    if (!peek(type))
    {
        auto message = fmt::format("expected {} but got {}",
            magic_enum::enum_name(type),
            magic_enum::enum_name(peek().type));

        throw FrontendError(peek(), std::move(message));
    }

    advance();

    return peek_last();
}

pars::Token pars::Lexer::peek()
{
    if (!m_current_token.has_value())
    {
        return advance();
    }

    return m_current_token.value();
}

bool pars::Lexer::peek(TokenType type)
{
    return peek().type == type;
}

pars::Token pars::Lexer::peek_next()
{
    if (m_next_token.has_value())
    {
        return m_next_token.value();
    }

    m_next_token = advance_one();

    return m_next_token.value();
}

bool pars::Lexer::peek_next(TokenType type)
{
    return peek_next().type == type;
}

pars::Token pars::Lexer::peek_last()
{
    return m_last_token.value_or(Token{});
}

bool pars::Lexer::peek_last(TokenType type)
{
    return peek_last().type == type;
}

bool pars::Lexer::has_next() const
{
    return !m_reader.at_end();
}

pars::Token pars::Lexer::build_token(TokenType type, std::string_view lexeme_override)
{
    return
    {
        .location =
        {
            .offset = m_reader.get_offset(),
            .line = m_reader.get_current_line(),
            .file_id = m_file_id,
            .column = m_reader.get_current_column(),
        },
        .type = type,
        .lexeme = lexeme_override.empty() ? m_reader.slice() : lexeme_override
    };
}

pars::Token pars::Lexer::build_token(char match, TokenType tk1, TokenType tk2)
{
    auto type = m_reader.match(match) ? tk1 : tk2;

    return build_token(type);
}

pars::Token pars::Lexer::build_error(std::string_view message)
{
    return build_token(TokenType::Error, message);
}

/*
 *  TODO: handle different types of integer literals
 *  TODO: hex 0x1
 *  TODO: octal 01
 *  TODO: binary 0b001
 *  TODO: floating point 1.0f 1.0L
 *  TODO: integer 1u 1UL
*/
pars::Token pars::Lexer::build_digit()
{
    auto scan_digits = [](TextReader &reader)
    {
        while (!reader.at_end() && std::isdigit(reader.peek()))
        {
            reader.advance();
        }
    };

    scan_digits(m_reader);

    auto type = TokenType::IntegerLiteral;

    if (m_reader.match('.'))
    {
        if (!std::isdigit(m_reader.peek()))
        {
            return build_error("expected digit after '.'");
        }

        type = TokenType::DecimalLiteral;
        scan_digits(m_reader);
    }

    return build_token(type);
}


char escaped_char(char c)
{
   switch (c)
   {
       case 'n': return '\n';
       case 't': return '\t';
       case '0': return '\0';
       case 'r': return '\r';
       case 'a': return '\a';
       case 'b': return '\b';
       case 'f': return '\f';
       case 'v': return '\v';
       case '\\': return '\\';
       case '\"': return '\"';
       case '\'': return '\'';
   }
}

// buffer shared by all escaped strings
static std::string g_escape_buffer;

/*
 * TODO: handle wide strings
 * TODO: add escape support for numeric values such as octal or hex (ive never had much of a use for this myself so likely wont implement for a while)
*/
pars::Token pars::Lexer::build_string()
{
    auto escape_buffer_start = UINT32_MAX;

    std::string_view lexeme;

    while (!m_reader.at_end() && m_reader.peek() != '"')
    {
        auto c = m_reader.advance();

        if (m_reader.is_escape_sequence())
        {
            if (escape_buffer_start == UINT32_MAX)
            {
                auto s = m_reader.slice();

                s = s.substr(0, s.size() - 2);

                escape_buffer_start = g_escape_buffer.size();

                g_escape_buffer += s;
            }

            g_escape_buffer += escaped_char(m_reader.peek_last());
        }
        else if (escape_buffer_start != UINT32_MAX)
        {
            g_escape_buffer += c;
        }
    }

    if (!m_reader.match('"'))
    {
        return build_error("unclosed string found");
    }

    size_t end_size;

    if (escape_buffer_start != UINT32_MAX)
    {
        lexeme = std::string_view{g_escape_buffer.begin() + escape_buffer_start, g_escape_buffer.end()};
        end_size = lexeme.size();
    }
    else
    {
        lexeme = m_reader.slice();
        end_size = lexeme.size()-2;
    }

    auto token = build_token(TokenType::StringLiteral, lexeme);

    token.lexeme = token.lexeme.substr(1, end_size);

    return token;
}

// TODO: handle wide chars
pars::Token pars::Lexer::build_char()
{
    m_reader.advance();

    std::string_view lexeme;

    if (m_reader.is_escape_sequence())
    {
        g_escape_buffer += escaped_char(m_reader.peek_last());
        lexeme = std::string_view{g_escape_buffer.begin() + g_escape_buffer.size() - 1, g_escape_buffer.end()};
    }
    else
    {
        lexeme = m_reader.slice();
        lexeme = lexeme.substr(1);
    }

    if (!m_reader.match('\''))
    {
        return build_error("unclosed or empty char found");
    }

    return build_token(TokenType::CharLiteral, lexeme);
}

pars::Token pars::Lexer::build_identifier()
{
    auto identifier = m_reader.get_identifier();
    auto token = build_token(TokenType::Identifier);

    token.lexeme = identifier;

    auto iter = g_keywords.find(identifier);

    if (iter != g_keywords.end())
    {
        token.type = iter->second;
    }

    return token;
}

