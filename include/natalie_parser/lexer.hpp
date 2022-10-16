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

    Lexer(const Lexer &other, char start_char, char stop_char)
        : m_input { other.m_input }
        , m_file { other.m_file }
        , m_size { other.m_size }
        , m_index { other.m_index }
        , m_cursor_line { other.m_cursor_line }
        , m_cursor_column { other.m_cursor_column }
        , m_token_line { other.m_token_line }
        , m_token_column { other.m_token_column }
        , m_stop_char { stop_char }
        , m_start_char { start_char } { }

    SharedPtr<Vector<Token>> tokens();
    Token next_token();

    virtual ~Lexer() {
        delete m_nested_lexer;
    }

    SharedPtr<String> file() const { return m_file; }

    size_t cursor_line() const { return m_cursor_line; }
    void set_cursor_line(size_t cursor_line) { m_cursor_line = cursor_line; }

    void set_nested_lexer(Lexer *lexer) { m_nested_lexer = lexer; }
    void set_start_char(char c) { m_start_char = c; }
    void set_stop_char(char c) { m_stop_char = c; }

    virtual bool alters_parent_cursor_position() { return true; }

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
    SharedPtr<String> consume_word();
    Token consume_word(Token::Type type);
    Token consume_bare_name_or_constant(Token::Type type);
    Token consume_global_variable();
    Token consume_heredoc();
    Token consume_numeric();
    Token consume_numeric_as_float(SharedPtr<String>);
    Token consume_nth_ref();
    long long consume_hex_number(int max_length = 0, bool allow_underscore = false);
    long long consume_octal_number(int max_length = 0, bool allow_underscore = false);
    Token consume_double_quoted_string(char, char, Token::Type begin_type, Token::Type end_type);
    Token consume_single_quoted_string(char, char);
    Token consume_quoted_array_without_interpolation(char start_char, char stop_char, Token::Type type);
    Token consume_quoted_array_with_interpolation(char start_char, char stop_char, Token::Type type);
    Token consume_regexp(char start_char, char stop_char);
    Token consume_percent_symbol(char start_char, char stop_char);
    Token consume_interpolated_string(char start_char, char stop_char);
    Token consume_interpolated_shell(char start_char, char stop_char);
    Token consume_percent_lower_w(char start_char, char stop_char);
    Token consume_percent_upper_w(char start_char, char stop_char);
    Token consume_percent_lower_i(char start_char, char stop_char);
    Token consume_percent_upper_i(char start_char, char stop_char);
    Token consume_percent_string(Token (Lexer::*consumer)(char start_char, char stop_char), bool is_lettered = true);
    SharedPtr<String> consume_non_whitespace();

    void utf32_codepoint_to_utf8(String &buf, long long codepoint);
    std::pair<bool, Token::Type> consume_escaped_byte(String &buf);

    Token chars_to_fixnum_or_bignum_token(SharedPtr<String> chars, int base, int offset);

    bool token_is_first_on_line() const;

    bool char_can_be_string_or_regexp_delimiter(char c) const {
        return (c >= '!' && c <= '/') || c == ':' || c == ';' || c == '=' || c == '?' || c == '@' || c == '\\' || c == '~' || c == '|' || (c >= '^' && c <= '`');
    }

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

    // we have an open ternary '?' that needs a matching ':'
    bool m_open_ternary { false };

    Lexer *m_nested_lexer { nullptr };

    char m_stop_char { 0 };

    // if we encounter the m_start_char within the string,
    // then increment m_pair_depth
    char m_start_char { 0 };
    int m_pair_depth { 0 };

    size_t m_remaining_method_names { 0 };
    bool m_allow_assignment_method { false };
    Token::Type m_method_name_separator { Token::Type::Invalid };
    Token m_last_method_name {};
};
}
