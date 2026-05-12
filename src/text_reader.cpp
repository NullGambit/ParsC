#include "text_reader.hpp"

#include <cctype>
#include <iostream>

pars::TextReader::TextReader(std::string_view source)
{
    set_source(source);
}

void pars::TextReader::set_source(std::string_view source)
{
    m_source = source;
    m_current = 0;
    m_offset = 0;
    m_current_line = 1;
    m_current_column = 1;
}

u8 pars::TextReader::advance()
{
    if (at_end())
    {
        return '\0';
    }

    auto c = m_source[m_offset++];

    if (c == '\n')
    {
        m_current_line++;
        m_current_column = 1;
    }
    else
    {
        m_current_column++;
    }

    return c;
}

bool pars::TextReader::at_end() const
{
    return m_offset >= m_source.size();
}

std::string_view pars::TextReader::slice()
{
    auto string = m_source.substr(m_current, m_offset - m_current);

    m_current = m_offset;

    return string;
}

std::string_view pars::TextReader::get_identifier()
{
    while (!at_end() && is_identifier(peek()))
    {
        advance();
    }

    return slice();
}

std::string_view pars::TextReader::get_until_blank_or(char c)
{
    sync();

    while (!at_end() && !is_blank(peek()) && peek_next() != c)
    {
        advance();
    }

    return slice();
}

std::string_view pars::TextReader::get_line()
{
    auto i = 0;

    while (!at_end() && peek() != '\n')
    {
        advance();
        i++;
    }

    return m_source.substr(m_current, i);
}

void pars::TextReader::skip_insignificant(bool blank_only)
{
    while (!at_end() && blank_only ? is_blank(peek()) : is_insignificant(peek()))
    {
        advance();
    }

    sync();
}

bool pars::TextReader::is_insignificant(u8 c)
{
    return is_blank(c) || c == '\r' || c == '\n';
}

bool pars::TextReader::is_blank(u8 c)
{
    return c == ' ' || c == '\t';
}

bool pars::TextReader::is_identifier(u8 c)
{
    return isalnum(c) || c == '_';
}

void pars::TextReader::skip_until(u8 c)
{
    while (!at_end() && peek() != c)
    {
        advance();
    }
}

bool pars::TextReader::match(char c)
{
    if (peek() == c)
    {
        advance();
        return true;
    }

    return false;
}

bool pars::TextReader::match_next(char c)
{
    if (peek_next() == c)
    {
        advance();
        return true;
    }

    return false;
}

void pars::TextReader::sync()
{
    m_current = m_offset;
}

u8 pars::TextReader::peek()
{
    if (at_end())
    {
        return '\0';
    }

    return m_source[m_offset];
}

u8 pars::TextReader::peek_next()
{
    if (m_offset + 1 > m_source.size())
    {
        return '\0';
    }

    return m_source[m_offset + 1];
}

u8 pars::TextReader::peek_last()
{
    if (m_offset - 1 <= 0)
    {
        return '\0';
    }

    return m_source[m_offset - 1];
}

bool pars::TextReader::is_escape_sequence()
{
    auto is_escape_start = peek_last() == '\\';

    return is_escape_start &&
        (
                match('n')
            ||  match('t')
            ||  match('0')
            ||  match('r')
            ||  match('a')
            ||  match('b')
            ||  match('f')
            ||  match('v')
            ||  match('\\')
            ||  match('\"')
            ||  match('\'')
        );
}
