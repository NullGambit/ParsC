#pragma once
#include <string_view>

namespace pars
{
    bool is_digit(std::string_view str);

    class TextReader
    {
    public:
        TextReader(std::string_view source);

        TextReader() = default;

        void set_source(std::string_view source);

        u8 advance();

        bool at_end() const;

        std::string_view slice();

        std::string_view get_identifier();

        std::string_view get_until_blank_or(char c);

        std::string_view get_line();

        void skip_insignificant(bool blank_only = false);

        static bool is_insignificant(u8 c);

        static bool is_blank(u8 c);

        static bool is_identifier(u8 c);

        void skip_until(u8 c);

        bool match(char c);

        bool match_next(char c);

        void sync();

        u8 peek();

        u8 peek_next();

        inline u32 get_offset() const
        {
            return m_offset;
        }

        inline u32 get_current_line() const
        {
            return m_current_line;
        }

        inline u16 get_current_column() const
        {
            return m_current_column;
        }

    private:
        u32 m_offset;
        u32 m_current;
        u32 m_current_line;
        u16 m_current_column;
        std::string_view m_source;
    };
}
