#pragma once

#include "natalie_parser/token.hpp"
#include "tm/shared_ptr.hpp"
#include "tm/vector.hpp"

namespace NatalieParser {

class Lexer {
public:
    Lexer(SharedPtr<String> input, SharedPtr<String> file)
        : m_input { input }
        , m_file { file }
        , m_size { input->length() } { }

    SharedPtr<Vector<Token>> tokens();
    Token next_token();

    virtual ~Lexer() = default;

    SharedPtr<String> file() const { return m_file; }

    void set_nested_lexer(Lexer *lexer) { m_nested_lexer = lexer; }
    void set_start_char(char c) { m_start_char = c; }
    void set_stop_char(char c) { m_stop_char = c; }

protected:
    char current_char() {
        if (m_index >= m_size)
            return 0;
        char c = m_input->at(m_index);
        return c;
    }

    bool match(size_t bytes, const char *compare);
    void advance();
    void advance(size_t bytes);
    void rewind(size_t bytes = 1);

    char next() {
        advance();
        return current_char();
    }

    char peek() {
        if (m_index + 1 >= m_size)
            return 0;
        return m_input->at(m_index + 1);
    }

    virtual bool skip_whitespace();
    virtual Token build_next_token();
    Token consume_symbol();
    Token consume_word(Token::Type type);
    Token consume_bare_name();
    Token consume_constant();
    Token consume_global_variable();
    Token consume_heredoc();
    Token consume_numeric();
    Token consume_numeric_as_float(SharedPtr<String>);
    Token consume_nth_ref();
    long long consume_hex_number(int max_length = 0, bool allow_underscore = false);
    long long consume_octal_number(int max_length = 0, bool allow_underscore = false);
    Token consume_double_quoted_string(char, char, Token::Type begin_type = Token::Type::InterpolatedStringBegin, Token::Type end_type = Token::Type::InterpolatedStringEnd);
    Token consume_single_quoted_string(char, char);
    Token consume_quoted_array_without_interpolation(char start_char, char stop_char, Token::Type type);
    Token consume_quoted_array_with_interpolation(char start_char, char stop_char, Token::Type type);
    Token consume_regexp(char delimiter);
    SharedPtr<String> consume_non_whitespace();
    void utf32_codepoint_to_utf8(String &buf, long long codepoint);
    Token chars_to_fixnum_or_bignum_token(SharedPtr<String> chars, int base, int offset);

    bool token_is_first_on_line() const;

    SharedPtr<String> m_input;
    SharedPtr<String> m_file;
    size_t m_size { 0 };
    size_t m_index { 0 };

    // where we should jump after each heredoc
    Vector<size_t> m_heredoc_stack {};

    // current character position
    size_t m_cursor_line { 0 };
    size_t m_cursor_column { 0 };

    // start of current token
    size_t m_token_line { 0 };
    size_t m_token_column { 0 };

    // if the current token is preceded by whitespace
    bool m_whitespace_precedes { false };

    // the previously-matched token
    Token m_last_token {};

    Lexer *m_nested_lexer { nullptr };

    char m_stop_char { 0 };

    // if we encounter the m_start_char within the string,
    // then increment m_pair_depth
    char m_start_char { 0 };
    int m_pair_depth { 0 };
};
}
