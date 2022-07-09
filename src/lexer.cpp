#include <errno.h>
#include <limits>
#include <stdlib.h>

#include "natalie_parser/lexer.hpp"
#include "natalie_parser/lexer/interpolated_string_lexer.hpp"
#include "natalie_parser/lexer/regexp_lexer.hpp"
#include "natalie_parser/lexer/word_array_lexer.hpp"
#include "natalie_parser/token.hpp"

namespace NatalieParser {

SharedPtr<Vector<Token>> Lexer::tokens() {
    SharedPtr<Vector<Token>> tokens = new Vector<Token> {};
    bool skip_next_newline = false;
    Token last_doc_token;
    for (;;) {
        auto token = next_token();
        if (token.is_comment())
            continue;

        if (token.is_doc()) {
            if (last_doc_token)
                last_doc_token.literal_string()->append(*token.literal_string());
            else
                last_doc_token = token;
            continue;
        }

        // get rid of newlines after certain tokens
        if (skip_next_newline) {
            if (token.is_newline())
                continue;
            else
                skip_next_newline = false;
        }

        // get rid of newlines before certain tokens
        while (token.can_follow_collapsible_newline() && !tokens->is_empty() && tokens->last().is_newline())
            tokens->pop();

        if (last_doc_token) {
            if (token.can_have_doc()) {
                token.set_doc(last_doc_token.literal_string());
                last_doc_token = {};
            } else if (!token.is_end_of_line()) {
                last_doc_token = {};
            }
        }

        tokens->push(token);

        m_last_token = token;

        if (token.is_eof())
            return tokens;
        if (!token.is_valid())
            return tokens;
        if (token.can_precede_collapsible_newline())
            skip_next_newline = true;
    };
    TM_UNREACHABLE();
}

Token Lexer::next_token() {
    if (m_nested_lexer) {
        auto token = m_nested_lexer->next_token();
        if (token.is_eof()) {
            if (m_nested_lexer->alters_parent_cursor_position()) {
                m_index = m_nested_lexer->m_index;
                m_cursor_line = m_nested_lexer->m_cursor_line;
                m_cursor_column = m_nested_lexer->m_cursor_column;
            }
            delete m_nested_lexer;
            m_nested_lexer = nullptr;
        } else {
            return token;
        }
    }
    m_whitespace_precedes = skip_whitespace();
    m_token_line = m_cursor_line;
    m_token_column = m_cursor_column;
    return build_next_token();
}

bool is_identifier_char(char c) {
    if (!c) return false;
    return isalnum(c) || c == '_' || (unsigned int)c >= 128;
}

bool is_message_suffix(char c) {
    if (!c) return false;
    return c == '?' || c == '!';
}

bool is_identifier_char_or_message_suffix(char c) {
    return is_identifier_char(c) || is_message_suffix(c);
}

bool Lexer::match(size_t bytes, const char *compare) {
    if (m_index + bytes > m_size)
        return false;
    if (strncmp(compare, m_input->c_str() + m_index, bytes) == 0) {
        if (m_index + bytes < m_size && is_identifier_char_or_message_suffix(m_input->at(m_index + bytes)))
            return false;
        advance(bytes);
        return true;
    }
    return false;
}

void Lexer::advance() {
    auto c = current_char();
    m_index++;
    if (c == '\n') {
        m_cursor_line++;
        m_cursor_column = 0;
    } else {
        m_cursor_column++;
    }
}

void Lexer::advance(size_t bytes) {
    for (size_t i = 0; i < bytes; i++) {
        advance();
    }
}

// NOTE: this does not work across lines
void Lexer::rewind(size_t bytes) {
    current_char();
    m_cursor_column -= bytes;
    m_index -= bytes;
}

bool Lexer::skip_whitespace() {
    bool whitespace_found = false;
    char c = current_char();
    while (c == ' ' || c == '\t' || (c == '\\' && peek() == '\n')) {
        whitespace_found = true;
        advance();
        if (c == '\\') advance();
        c = current_char();
    }
    return whitespace_found;
}

Token Lexer::build_next_token() {
    if (m_index >= m_size)
        return Token { Token::Type::Eof, m_file, m_cursor_line, m_cursor_column, m_whitespace_precedes };
    if (m_start_char && current_char() == m_start_char) {
        m_pair_depth++;
    } else if (m_stop_char && current_char() == m_stop_char) {
        if (m_pair_depth == 0)
            return Token { Token::Type::Eof, m_file, m_cursor_line, m_cursor_column, m_whitespace_precedes };
        m_pair_depth--;
    } else if (m_index == 0 && current_char() == '\xEF') {
        // UTF-8 BOM
        advance(); // \xEF
        if (current_char() == '\xBB') advance();
        if (current_char() == '\xBF') advance();
    }
    Token token;
    switch (current_char()) {
    case '=': {
        advance();
        switch (current_char()) {
        case '=': {
            advance();
            switch (current_char()) {
            case '=': {
                advance();
                return Token { Token::Type::EqualEqualEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
            default:
                return Token { Token::Type::EqualEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        }
        case '>':
            advance();
            return Token { Token::Type::HashRocket, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        case '~':
            advance();
            return Token { Token::Type::Match, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        default:
            if (m_cursor_column == 1 && match(5, "begin")) {
                SharedPtr<String> doc = new String("=begin");
                char c = current_char();
                do {
                    doc->append_char(c);
                    c = next();
                } while (c && !(m_cursor_column == 0 && match(4, "=end")));
                doc->append("=end\n");
                return Token { Token::Type::Doc, doc, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
            auto token = Token { Token::Type::Equal, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            return token;
        }
    }
    case '+':
        advance();
        switch (current_char()) {
        case '=':
            advance();
            return Token { Token::Type::PlusEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        case '@':
            if (m_last_token.is_def_keyword() || m_last_token.is_dot()) {
                advance();
                SharedPtr<String> lit = new String("+@");
                return Token { Token::Type::BareName, lit, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            } else {
                return Token { Token::Type::Plus, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        default:
            return Token { Token::Type::Plus, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        }
    case '-':
        advance();
        switch (current_char()) {
        case '>':
            advance();
            return Token { Token::Type::Arrow, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        case '=':
            advance();
            return Token { Token::Type::MinusEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        case '@':
            if (m_last_token.is_def_keyword() || m_last_token.is_dot()) {
                advance();
                SharedPtr<String> lit = new String("-@");
                return Token { Token::Type::BareName, lit, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            } else {
                return Token { Token::Type::Minus, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        default:
            return Token { Token::Type::Minus, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        }
    case '*':
        advance();
        switch (current_char()) {
        case '*':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::StarStarEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            default:
                return Token { Token::Type::StarStar, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        case '=':
            advance();
            return Token { Token::Type::StarEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        default:
            return Token { Token::Type::Star, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        }
    case '/': {
        advance();
        if (!m_last_token)
            return consume_regexp('/', '/');
        switch (m_last_token.type()) {
        case Token::Type::Comma:
        case Token::Type::Doc:
        case Token::Type::LBracket:
        case Token::Type::LCurlyBrace:
        case Token::Type::LParen:
        case Token::Type::Match:
        case Token::Type::Newline:
            return consume_regexp('/', '/');
        case Token::Type::DefKeyword:
            return Token { Token::Type::Slash, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        default: {
            switch (current_char()) {
            case ' ':
                if (m_last_token.is_keyword() && m_last_token.can_precede_regexp_literal()) {
                    return consume_regexp('/', '/');
                } else {
                    return Token { Token::Type::Slash, m_file, m_token_line, m_token_column, m_whitespace_precedes };
                }
            case '=':
                advance();
                return Token { Token::Type::SlashEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            default:
                if (m_whitespace_precedes) {
                    return consume_regexp('/', '/');
                } else {
                    return Token { Token::Type::Slash, m_file, m_token_line, m_token_column, m_whitespace_precedes };
                }
            }
        }
        }
    }
    case '%':
        advance();
        switch (current_char()) {
        case '=':
            advance();
            return Token { Token::Type::PercentEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        case 'q':
            switch (peek()) {
            case '[':
                advance(2);
                return consume_single_quoted_string('[', ']');
            case '{':
                advance(2);
                return consume_single_quoted_string('{', '}');
            case '<':
                advance(2);
                return consume_single_quoted_string('<', '>');
            case '(':
                advance(2);
                return consume_single_quoted_string('(', ')');
            default: {
                char c = peek();
                if (char_can_be_string_or_regexp_delimiter(c)) {
                    advance(2);
                    return consume_single_quoted_string(c, c);
                } else {
                    return Token { Token::Type::Percent, m_file, m_token_line, m_token_column, m_whitespace_precedes };
                }
            }
            }
        case 'Q':
            switch (peek()) {
            case '[':
                advance(2);
                return consume_double_quoted_string('[', ']');
            case '{':
                advance(2);
                return consume_double_quoted_string('{', '}');
            case '<':
                advance(2);
                return consume_double_quoted_string('<', '>');
            case '(':
                advance(2);
                return consume_double_quoted_string('(', ')');
            default: {
                char c = peek();
                if (char_can_be_string_or_regexp_delimiter(c)) {
                    advance(2);
                    return consume_double_quoted_string(c, c);
                } else {
                    return Token { Token::Type::Percent, m_file, m_token_line, m_token_column, m_whitespace_precedes };
                }
            }
            }
        case 'r':
            switch (peek()) {
            case '[':
                advance(2);
                return consume_regexp('[', ']');
            case '{':
                advance(2);
                return consume_regexp('{', '}');
            case '(':
                advance(2);
                return consume_regexp('(', ')');
            case '<':
                advance(2);
                return consume_regexp('<', '>');
            default: {
                char c = peek();
                if (char_can_be_string_or_regexp_delimiter(c)) {
                    advance(2);
                    return consume_regexp(c, c);
                } else {
                    return Token { Token::Type::Percent, m_file, m_token_line, m_token_column, m_whitespace_precedes };
                }
            }
            }
        case 'x':
            switch (peek()) {
            case '/': {
                advance(2);
                return consume_double_quoted_string('/', '/', Token::Type::InterpolatedShellBegin, Token::Type::InterpolatedShellEnd);
            }
            case '[': {
                advance(2);
                return consume_double_quoted_string('[', ']', Token::Type::InterpolatedShellBegin, Token::Type::InterpolatedShellEnd);
            }
            case '{': {
                advance(2);
                return consume_double_quoted_string('{', '}', Token::Type::InterpolatedShellBegin, Token::Type::InterpolatedShellEnd);
            }
            case '(': {
                advance(2);
                return consume_double_quoted_string('(', ')', Token::Type::InterpolatedShellBegin, Token::Type::InterpolatedShellEnd);
            }
            default:
                return Token { Token::Type::Percent, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        case 'w':
            switch (peek()) {
            case '/':
            case '|': {
                char c = next();
                advance();
                return consume_quoted_array_without_interpolation(c, c, Token::Type::PercentLowerW);
            }
            case '[':
                advance(2);
                return consume_quoted_array_without_interpolation('[', ']', Token::Type::PercentLowerW);
            case '{':
                advance(2);
                return consume_quoted_array_without_interpolation('{', '}', Token::Type::PercentLowerW);
            case '<':
                advance(2);
                return consume_quoted_array_without_interpolation('<', '>', Token::Type::PercentLowerW);
            case '(':
                advance(2);
                return consume_quoted_array_without_interpolation('(', ')', Token::Type::PercentLowerW);
            default:
                return Token { Token::Type::Percent, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        case 'W':
            switch (peek()) {
            case '/':
            case '|': {
                char c = next();
                advance();
                return consume_quoted_array_with_interpolation(0, c, Token::Type::PercentUpperW);
            }
            case '[':
                advance(2);
                return consume_quoted_array_with_interpolation('[', ']', Token::Type::PercentUpperW);
            case '{':
                advance(2);
                return consume_quoted_array_with_interpolation('{', '}', Token::Type::PercentUpperW);
            case '<':
                advance(2);
                return consume_quoted_array_with_interpolation('<', '>', Token::Type::PercentUpperW);
            case '(':
                advance(2);
                return consume_quoted_array_with_interpolation('(', ')', Token::Type::PercentUpperW);
            default:
                return Token { Token::Type::Percent, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        case 'i':
            switch (peek()) {
            case '|':
            case '/': {
                char c = next();
                advance();
                return consume_quoted_array_without_interpolation(c, c, Token::Type::PercentLowerI);
            }
            case '[':
                advance(2);
                return consume_quoted_array_without_interpolation('[', ']', Token::Type::PercentLowerI);
            case '{':
                advance(2);
                return consume_quoted_array_without_interpolation('{', '}', Token::Type::PercentLowerI);
            case '<':
                advance(2);
                return consume_quoted_array_without_interpolation('<', '>', Token::Type::PercentLowerI);
            case '(':
                advance(2);
                return consume_quoted_array_without_interpolation('(', ')', Token::Type::PercentLowerI);
            default:
                return Token { Token::Type::Percent, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        case 'I':
            switch (peek()) {
            case '|':
            case '/': {
                char c = next();
                advance();
                return consume_quoted_array_with_interpolation(0, c, Token::Type::PercentUpperI);
            }
            case '[':
                advance(2);
                return consume_quoted_array_with_interpolation('[', ']', Token::Type::PercentUpperI);
            case '{':
                advance(2);
                return consume_quoted_array_with_interpolation('{', '}', Token::Type::PercentUpperI);
            case '<':
                advance(2);
                return consume_quoted_array_with_interpolation('<', '>', Token::Type::PercentUpperI);
            case '(':
                advance(2);
                return consume_quoted_array_with_interpolation('(', ')', Token::Type::PercentUpperI);
            default:
                return Token { Token::Type::Percent, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        case '[':
            advance();
            return consume_double_quoted_string('[', ']');
        case '{':
            advance();
            return consume_double_quoted_string('{', '}');
        case '<':
            advance();
            return consume_double_quoted_string('<', '>');
        case '(':
            if (m_last_token.type() == Token::Type::DefKeyword || m_last_token.type() == Token::Type::Dot) {
                // It's a trap! This looks like a %(string) but it's a method def/call!
                break;
            }
            advance();
            return consume_double_quoted_string('(', ')');
        default: {
            auto c = current_char();
            if (char_can_be_string_or_regexp_delimiter(c)) {
                advance();
                return consume_double_quoted_string(c, c);
            }
            break;
        }
        }
        return Token { Token::Type::Percent, m_file, m_token_line, m_token_column, m_whitespace_precedes };
    case '!':
        advance();
        switch (current_char()) {
        case '=':
            advance();
            return Token { Token::Type::NotEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        case '~':
            advance();
            return Token { Token::Type::NotMatch, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        case '@':
            if (m_last_token.is_def_keyword() || m_last_token.is_dot()) {
                advance();
                SharedPtr<String> lit = new String("!@");
                return Token { Token::Type::BareName, lit, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            } else {
                return Token { Token::Type::Not, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        default:
            return Token { Token::Type::Not, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        }
    case '<':
        advance();
        switch (current_char()) {
        case '<': {
            advance();
            switch (current_char()) {
            case '~':
            case '-': {
                auto next = peek();
                if (isalpha(next))
                    return consume_heredoc();
                switch (next) {
                case '_':
                case '"':
                case '`':
                case '\'':
                    return consume_heredoc();
                default:
                    return Token { Token::Type::LeftShift, m_file, m_token_line, m_token_column, m_whitespace_precedes };
                }
            }
            case '=':
                advance();
                return Token { Token::Type::LeftShiftEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            default:
                if (!m_whitespace_precedes) {
                    if (token_is_first_on_line())
                        return consume_heredoc();
                    else if (m_last_token.can_precede_heredoc_that_looks_like_left_shift_operator())
                        return consume_heredoc();
                    else
                        return Token { Token::Type::LeftShift, m_file, m_token_line, m_token_column, m_whitespace_precedes };
                }
                if (isalpha(current_char()))
                    return consume_heredoc();
                switch (current_char()) {
                case '_':
                case '"':
                case '`':
                case '\'':
                    return consume_heredoc();
                default:
                    return Token { Token::Type::LeftShift, m_file, m_token_line, m_token_column, m_whitespace_precedes };
                }
            }
        }
        case '=':
            advance();
            switch (current_char()) {
            case '>':
                advance();
                return Token { Token::Type::Comparison, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            default:
                return Token { Token::Type::LessThanOrEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        default:
            return Token { Token::Type::LessThan, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        }
    case '>':
        advance();
        switch (current_char()) {
        case '>':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::RightShiftEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            default:
                return Token { Token::Type::RightShift, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        case '=':
            advance();
            return Token { Token::Type::GreaterThanOrEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        default:
            return Token { Token::Type::GreaterThan, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        }
    case '&':
        advance();
        switch (current_char()) {
        case '&':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::AmpersandAmpersandEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            default:
                return Token { Token::Type::AmpersandAmpersand, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        case '=':
            advance();
            return Token { Token::Type::AmpersandEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        case '.':
            advance();
            return Token { Token::Type::SafeNavigation, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        default:
            return Token { Token::Type::Ampersand, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        }
    case '|':
        advance();
        switch (current_char()) {
        case '|':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::PipePipeEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            default:
                return Token { Token::Type::PipePipe, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        case '=':
            advance();
            return Token { Token::Type::PipeEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        default:
            return Token { Token::Type::Pipe, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        }
    case '^':
        advance();
        switch (current_char()) {
        case '=':
            advance();
            return Token { Token::Type::CaretEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        default:
            return Token { Token::Type::Caret, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        }
    case '~':
        advance();
        switch (current_char()) {
        case '@':
            if (m_last_token.is_def_keyword() || m_last_token.is_dot()) {
                advance();
                SharedPtr<String> lit = new String("~@");
                return Token { Token::Type::BareName, lit, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            } else {
                return Token { Token::Type::Tilde, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        default:
            return Token { Token::Type::Tilde, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        }
    case '?': {
        auto c = next();
        if (isspace(c)) {
            m_open_ternary = true;
            return Token { Token::Type::TernaryQuestion, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        } else {
            advance();
            if (c == '\\') {
                auto buf = new String();
                auto result = consume_escaped_byte(*buf);
                if (!result.first)
                    return Token { result.second, current_char(), m_file, m_token_line, m_token_column, m_whitespace_precedes };
                return Token { Token::Type::String, buf, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            } else {
                return Token { Token::Type::String, c, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        }
    }
    case ':': {
        auto c = next();
        if (c == ':') {
            advance();
            return Token { Token::Type::ConstantResolution, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        } else if (m_last_token.type() == Token::Type::InterpolatedStringEnd && !m_whitespace_precedes && !m_open_ternary) {
            return Token { Token::Type::InterpolatedStringSymbolKey, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        } else if (c == '"') {
            advance();
            return consume_double_quoted_string('"', '"', Token::Type::InterpolatedSymbolBegin, Token::Type::InterpolatedSymbolEnd);
        } else if (c == '\'') {
            advance();
            auto string = consume_single_quoted_string('\'', '\'');
            return Token { Token::Type::Symbol, string.literal(), m_file, m_token_line, m_token_column, m_whitespace_precedes };
        } else if (isspace(c)) {
            m_open_ternary = false;
            auto token = Token { Token::Type::TernaryColon, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            return token;
        } else {
            return consume_symbol();
        }
    }
    case '@':
        switch (peek()) {
        case '@': {
            // kinda janky, but we gotta trick consume_word and then prepend the '@' back on the front
            advance();
            auto token = consume_word(Token::Type::ClassVariable);
            token.set_literal(String::format("@{}", token.literal()));
            return token;
        }
        default:
            return consume_word(Token::Type::InstanceVariable);
        }
    case '$':
        if (peek() == '&') {
            advance(2);
            return Token { Token::Type::BackRef, '&', m_file, m_token_line, m_token_column, m_whitespace_precedes };
        } else if (peek() >= '1' && peek() <= '9') {
            return consume_nth_ref();
        } else {
            return consume_global_variable();
        }
    case '.':
        advance();
        switch (current_char()) {
        case '.':
            advance();
            switch (current_char()) {
            case '.':
                advance();
                return Token { Token::Type::DotDotDot, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            default:
                return Token { Token::Type::DotDot, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        default:
            return Token { Token::Type::Dot, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        }
    case '{':
        advance();
        return Token { Token::Type::LCurlyBrace, m_file, m_token_line, m_token_column, m_whitespace_precedes };
    case '[': {
        advance();
        switch (current_char()) {
        case ']':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::LBracketRBracketEqual, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            default:
                auto token = Token { Token::Type::LBracketRBracket, m_file, m_token_line, m_token_column, m_whitespace_precedes };
                return token;
            }
        default:
            auto token = Token { Token::Type::LBracket, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            return token;
        }
    }
    case '(': {
        advance();
        auto token = Token { Token::Type::LParen, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        return token;
    }
    case '}':
        advance();
        return Token { Token::Type::RCurlyBrace, m_file, m_token_line, m_token_column, m_whitespace_precedes };
    case ']':
        advance();
        return Token { Token::Type::RBracket, m_file, m_token_line, m_token_column, m_whitespace_precedes };
    case ')':
        advance();
        return Token { Token::Type::RParen, m_file, m_token_line, m_token_column, m_whitespace_precedes };
    case '\n': {
        advance();
        auto token = Token { Token::Type::Newline, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        if (!m_heredoc_stack.is_empty()) {
            auto new_index = m_heredoc_stack.last();
            while (m_index < new_index)
                advance();
            m_heredoc_stack.clear();
        }
        return token;
    }
    case ';':
        advance();
        return Token { Token::Type::Semicolon, m_file, m_token_line, m_token_column, m_whitespace_precedes };
    case ',':
        advance();
        return Token { Token::Type::Comma, m_file, m_token_line, m_token_column, m_whitespace_precedes };
    case '"':
        advance();
        return consume_double_quoted_string('"', '"');
    case '\'':
        advance();
        return consume_single_quoted_string('\'', '\'');
    case '`': {
        advance();
        return consume_double_quoted_string('`', '`', Token::Type::InterpolatedShellBegin, Token::Type::InterpolatedShellEnd);
    }
    case '#':
        if (token_is_first_on_line()) {
            SharedPtr<String> doc = new String();
            bool found_comment_marker = true;
            char c = current_char();
            while (c) {
                if (!found_comment_marker) {
                    if (c == '#')
                        found_comment_marker = true;
                    else if (!isspace(c))
                        break;
                }
                if (c == '\n' || c == '\r') {
                    doc->append_char(c);
                    found_comment_marker = false;
                } else if (found_comment_marker)
                    doc->append_char(c);
                c = next();
            }
            return Token { Token::Type::Doc, doc, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        } else {
            char c;
            do {
                c = next();
            } while (c && c != '\n' && c != '\r');
            return Token { Token::Type::Comment, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        }
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
        auto token = consume_numeric();
        return token;
    }
    case 'i':
        if (m_last_token.can_be_complex_or_rational() && !isalnum(peek())) {
            advance();
            return Token { Token::Type::Complex, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        }
        break;
    case 'r':
        if (m_last_token.can_be_complex_or_rational()) {
            if (peek() == 'i') {
                advance(2);
                return Token { Token::Type::RationalComplex, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            } else if (!isalnum(peek())) {
                advance();
                return Token { Token::Type::Rational, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        }
        break;
    };

    Token keyword_token;

    if (!m_last_token.is_dot() && match(4, "self")) {
        if (current_char() == '.')
            keyword_token = { Token::Type::SelfKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else
            rewind(4);
    }

    if (!m_last_token.is_dot() && !m_last_token.is_def_keyword()) {
        if (match(12, "__ENCODING__"))
            keyword_token = { Token::Type::ENCODINGKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(8, "__LINE__"))
            keyword_token = { Token::Type::LINEKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(8, "__FILE__"))
            keyword_token = { Token::Type::FILEKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(5, "BEGIN"))
            keyword_token = { Token::Type::BEGINKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(3, "END"))
            keyword_token = { Token::Type::ENDKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(5, "alias"))
            keyword_token = { Token::Type::AliasKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(3, "and"))
            keyword_token = { Token::Type::AndKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(5, "begin"))
            keyword_token = { Token::Type::BeginKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(5, "break"))
            keyword_token = { Token::Type::BreakKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(4, "case"))
            keyword_token = { Token::Type::CaseKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(5, "class"))
            keyword_token = { Token::Type::ClassKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(8, "defined?"))
            keyword_token = { Token::Type::DefinedKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(3, "def"))
            keyword_token = { Token::Type::DefKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(2, "do"))
            keyword_token = { Token::Type::DoKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(4, "else"))
            keyword_token = { Token::Type::ElseKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(5, "elsif"))
            keyword_token = { Token::Type::ElsifKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(3, "end"))
            keyword_token = { Token::Type::EndKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(6, "ensure"))
            keyword_token = { Token::Type::EnsureKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(5, "false"))
            keyword_token = { Token::Type::FalseKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(3, "for"))
            keyword_token = { Token::Type::ForKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(2, "if"))
            keyword_token = { Token::Type::IfKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(2, "in"))
            keyword_token = { Token::Type::InKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(6, "module"))
            keyword_token = { Token::Type::ModuleKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(4, "next"))
            keyword_token = { Token::Type::NextKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(3, "nil"))
            keyword_token = { Token::Type::NilKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(3, "not"))
            keyword_token = { Token::Type::NotKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(2, "or"))
            keyword_token = { Token::Type::OrKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(4, "redo"))
            keyword_token = { Token::Type::RedoKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(6, "rescue"))
            keyword_token = { Token::Type::RescueKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(5, "retry"))
            keyword_token = { Token::Type::RetryKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(6, "return"))
            keyword_token = { Token::Type::ReturnKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(4, "self"))
            keyword_token = { Token::Type::SelfKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(5, "super"))
            keyword_token = { Token::Type::SuperKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(4, "then"))
            keyword_token = { Token::Type::ThenKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(4, "true"))
            keyword_token = { Token::Type::TrueKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(5, "undef"))
            keyword_token = { Token::Type::UndefKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(6, "unless"))
            keyword_token = { Token::Type::UnlessKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(5, "until"))
            keyword_token = { Token::Type::UntilKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(4, "when"))
            keyword_token = { Token::Type::WhenKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(5, "while"))
            keyword_token = { Token::Type::WhileKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        else if (match(5, "yield"))
            keyword_token = { Token::Type::YieldKeyword, m_file, m_token_line, m_token_column, m_whitespace_precedes };
    }

    // if a colon comes next, it's not a keyword -- it's a symbol!
    if (keyword_token && current_char() == ':' && peek() != ':' && !m_open_ternary) {
        advance(); // :
        auto name = keyword_token.type_value();
        return Token { Token::Type::SymbolKey, name, m_file, m_token_line, m_token_column, m_whitespace_precedes };
    } else if (keyword_token) {
        return keyword_token;
    }

    auto c = current_char();
    if ((c >= 'a' && c <= 'z') || c == '_') {
        return consume_bare_name();
    } else if (c >= 'A' && c <= 'Z') {
        return consume_constant();
    } else {
        auto buf = consume_non_whitespace();
        auto token = Token { Token::Type::Invalid, buf, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        return token;
    }

    TM_UNREACHABLE();
}

Token Lexer::consume_symbol() {
    char c = current_char();
    SharedPtr<String> buf = new String("");
    auto gobble = [&buf, this](char c) -> char { buf->append_char(c); return next(); };
    switch (c) {
    case '@':
        c = gobble(c);
        if (c == '@') c = gobble(c);
        do {
            c = gobble(c);
        } while (is_identifier_char(c));
        break;
    case '$':
        c = gobble(c);
        do {
            c = gobble(c);
        } while (is_identifier_char(c));
        break;
    case '~':
        c = gobble(c);
        if (c == '@') advance();
        break;
    case '+':
    case '-': {
        c = gobble(c);
        if (c == '@') gobble(c);
        break;
    }
    case '&':
    case '|':
    case '^':
    case '%':
    case '/': {
        gobble(c);
        break;
    }
    case '*':
        c = gobble(c);
        if (c == '*')
            gobble(c);
        break;
    case '=':
        switch (peek()) {
        case '=':
            c = gobble(c);
            c = gobble(c);
            if (c == '=') gobble(c);
            break;
        case '~':
            c = gobble(c);
            gobble(c);
            break;
        default:
            return Token { Token::Type::Invalid, c, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        }
        break;
    case '!':
        c = gobble(c);
        switch (c) {
        case '=':
        case '~':
        case '@':
            gobble(c);
        default:
            break;
        }
        break;
    case '>':
        c = gobble(c);
        switch (c) {
        case '=':
        case '>':
            gobble(c);
        default:
            break;
        }
        break;
    case '<':
        c = gobble(c);
        switch (c) {
        case '=':
            c = gobble(c);
            if (c == '>') gobble(c);
            break;
        case '<':
            gobble(c);
        default:
            break;
        }
        break;
    case '[':
        if (peek() == ']') {
            c = gobble(c);
            c = gobble(c);
            if (c == '=') gobble(c);
        } else {
            return Token { Token::Type::Invalid, c, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        }
        break;
    default:
        do {
            c = gobble(c);
        } while (is_identifier_char(c));
        switch (c) {
        case '?':
        case '!':
        case '=':
            switch (peek()) {
            case '>':
                break;
            default:
                gobble(c);
            }
        default:
            break;
        }
    }
    return Token { Token::Type::Symbol, buf, m_file, m_token_line, m_token_column, m_whitespace_precedes };
}

Token Lexer::consume_word(Token::Type type) {
    char c = current_char();
    SharedPtr<String> buf = new String("");
    do {
        buf->append_char(c);
        c = next();
    } while (is_identifier_char(c));
    switch (c) {
    case '?':
    case '!':
        advance();
        buf->append_char(c);
        break;
    default:
        break;
    }
    return Token { type, buf, m_file, m_token_line, m_token_column, m_whitespace_precedes };
}

Token Lexer::consume_bare_name() {
    auto token = consume_word(Token::Type::BareName);
    auto c = current_char();
    if (c == ':' && peek() != ':' && m_last_token.can_precede_symbol_key()) {
        advance();
        token.set_type(Token::Type::SymbolKey);
    }
    return token;
}

Token Lexer::consume_constant() {
    auto token = consume_word(Token::Type::Constant);
    auto c = current_char();
    if (c == ':' && peek() != ':' && m_last_token.can_precede_symbol_key()) {
        advance();
        token.set_type(Token::Type::SymbolKey);
    }
    return token;
}

Token Lexer::consume_global_variable() {
    switch (peek()) {
    case '?':
    case '!':
    case '=':
    case '@':
    case '&':
    case '`':
    case '\'':
    case '"':
    case '+':
    case '/':
    case '\\':
    case ';':
    case '<':
    case '>':
    case '$':
    case '*':
    case '.':
    case ',':
    case ':':
    case '_':
    case '~': {
        advance();
        SharedPtr<String> buf = new String("$");
        buf->append_char(current_char());
        advance();
        return Token { Token::Type::GlobalVariable, buf, m_file, m_token_line, m_token_column, m_whitespace_precedes };
    }
    case '-': {
        SharedPtr<String> buf = new String("$-");
        advance(2);
        buf->append_char(current_char());
        advance();
        return Token { Token::Type::GlobalVariable, buf, m_file, m_token_line, m_token_column, m_whitespace_precedes };
    }
    default: {
        return consume_word(Token::Type::GlobalVariable);
    }
    }
}

bool is_valid_heredoc(bool with_dash, SharedPtr<String> doc, String heredoc_name) {
    if (!doc->ends_with(heredoc_name))
        return false;
    if (doc->length() - heredoc_name.length() == 0)
        return true;
    auto prefix = (*doc)[doc->length() - heredoc_name.length() - 1];
    return with_dash ? isspace(prefix) : prefix == '\n';
}

size_t get_heredoc_indent(SharedPtr<String> doc) {
    if (doc->is_empty())
        return 0;
    size_t heredoc_indent = std::numeric_limits<size_t>::max();
    size_t line_indent = 0;
    bool maybe_blank_line = true;
    for (size_t i = 0; i < doc->length(); i++) {
        char c = (*doc)[i];
        if (c == '\n') {
            if (!maybe_blank_line && line_indent < heredoc_indent)
                heredoc_indent = line_indent;
            line_indent = 0;
            maybe_blank_line = true;
        } else if (isspace(c)) {
            if (maybe_blank_line)
                line_indent++;
        } else {
            maybe_blank_line = false;
        }
    }
    return heredoc_indent;
}

void dedent_heredoc(SharedPtr<String> &doc) {
    size_t heredoc_indent = get_heredoc_indent(doc);
    if (heredoc_indent == 0)
        return;
    SharedPtr<String> new_doc = new String("");
    size_t line_begin = 0;
    for (size_t i = 0; i < doc->length(); i++) {
        char c = (*doc)[i];
        if (c == '\n') {
            line_begin += heredoc_indent;
            if (line_begin < i)
                new_doc->append(doc->substring(line_begin, i - line_begin));
            new_doc->append_char('\n');
            line_begin = i + 1;
        }
    }
    doc = new_doc;
}

Token Lexer::consume_heredoc() {
    bool with_dash = false;
    bool should_dedent = false;
    switch (current_char()) {
    case '-':
        advance();
        with_dash = true;
        break;
    case '~':
        advance();
        with_dash = true;
        should_dedent = true;
        break;
    }

    auto begin_type = Token::Type::InterpolatedStringBegin;
    auto end_type = Token::Type::InterpolatedStringEnd;
    bool should_interpolate = true;
    char delimiter = 0;
    String heredoc_name = "";
    switch (current_char()) {
    case '"':
        delimiter = '"';
        break;
    case '`':
        begin_type = Token::Type::InterpolatedShellBegin;
        end_type = Token::Type::InterpolatedShellEnd;
        delimiter = '`';
        break;
    case '\'':
        should_interpolate = false;
        delimiter = '\'';
        break;
    default:
        delimiter = 0;
    }

    if (delimiter) {
        char c = next();
        while (c != delimiter) {
            switch (c) {
            case '\n':
            case '\r':
            case 0:
                return Token { Token::Type::UnterminatedString, "heredoc identifier", m_file, m_token_line, m_token_column, m_whitespace_precedes };
            default:
                heredoc_name.append_char(c);
                c = next();
            }
        }
        advance();
    } else {
        heredoc_name = String(consume_word(Token::Type::BareName).literal());
    }

    SharedPtr<String> doc = new String("");
    size_t heredoc_index = m_index;
    auto get_char = [&heredoc_index, this]() { return (heredoc_index >= m_size) ? 0 : m_input->at(heredoc_index); };

    if (m_heredoc_stack.is_empty()) {
        // start consuming the heredoc on the next line
        while (get_char() != '\n') {
            if (heredoc_index >= m_size)
                return Token { Token::Type::UnterminatedString, "heredoc", m_file, m_token_line, m_token_column, m_whitespace_precedes };
            heredoc_index++;
        }
        heredoc_index++;
    } else {
        // start consuming the heredoc right after the last one
        heredoc_index = m_heredoc_stack.last();
    }

    // consume the heredoc until we find the delimiter, either '\n' (if << was used) or any whitespace (if <<- was used) followed by "DELIM\n"
    for (;;) {
        if (heredoc_index >= m_size) {
            if (is_valid_heredoc(with_dash, doc, heredoc_name))
                break;
            return Token { Token::Type::UnterminatedString, doc, m_file, m_token_line, m_token_column, m_whitespace_precedes };
        }
        char c = get_char();
        heredoc_index++;
        if (c == '\n' && is_valid_heredoc(with_dash, doc, heredoc_name))
            break;
        doc->append_char(c);
    }

    // chop the delimiter and any trailing space off the string
    doc->truncate(doc->length() - heredoc_name.length());
    doc->strip_trailing_spaces();

    if (should_dedent)
        dedent_heredoc(doc);

    // We have to keep tokenizing on the line where the heredoc was started, and then jump to the line after the heredoc.
    // This index is used to jump to the end of the heredoc later.
    m_heredoc_stack.push(heredoc_index);

    auto token = Token { Token::Type::String, doc, m_file, m_token_line, m_token_column, m_whitespace_precedes };

    if (should_interpolate) {
        m_nested_lexer = new InterpolatedStringLexer { *this, token, end_type };
        return Token { begin_type, m_file, m_token_line, m_token_column, m_whitespace_precedes };
    }

    return token;
}

Token Lexer::consume_numeric() {
    SharedPtr<String> chars = new String;

    auto consume_decimal_digits_and_build_token = [&]() {
        char c = current_char();
        do {
            chars->append_char(c);
            c = next();
            if (c == '_')
                c = next();
        } while (isdigit(c));
        if ((c == '.' && isdigit(peek())) || (c == 'e' || c == 'E'))
            return consume_numeric_as_float(chars);
        else
            return chars_to_fixnum_or_bignum_token(chars, 10, 0);
    };

    Token token;

    if (current_char() == '0') {
        // special-prefixed literals 0d, 0x, etc.
        switch (peek()) {
        case 'd':
        case 'D': {
            advance();
            char c = next();
            if (!isdigit(c))
                return Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column, m_whitespace_precedes };
            do {
                chars->append_char(c);
                c = next();
                if (c == '_')
                    c = next();
            } while (isdigit(c));
            token = chars_to_fixnum_or_bignum_token(chars, 10, 0);
            break;
        }
        case 'o':
        case 'O': {
            chars->append_char('0');
            chars->append_char('o');
            advance();
            char c = next();
            if (!(c >= '0' && c <= '7'))
                return Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column, m_whitespace_precedes };
            do {
                chars->append_char(c);
                c = next();
                if (c == '_')
                    c = next();
            } while (c >= '0' && c <= '7');
            token = chars_to_fixnum_or_bignum_token(chars, 8, 2);
            break;
        }
        case 'x':
        case 'X': {
            chars->append_char('0');
            chars->append_char('x');
            advance();
            char c = next();
            if (!isxdigit(c))
                return Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column, m_whitespace_precedes };
            do {
                chars->append_char(c);
                c = next();
                if (c == '_')
                    c = next();
            } while (isxdigit(c));
            token = chars_to_fixnum_or_bignum_token(chars, 16, 2);
            break;
        }
        case 'b':
        case 'B': {
            chars->append_char('0');
            chars->append_char('b');
            advance();
            char c = next();
            if (c != '0' && c != '1')
                return Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column, m_whitespace_precedes };
            do {
                chars->append_char(c);
                c = next();
                if (c == '_')
                    c = next();
            } while (c == '0' || c == '1');
            token = chars_to_fixnum_or_bignum_token(chars, 2, 2);
            break;
        }
        default:
            token = consume_decimal_digits_and_build_token();
        }
    } else {
        token = consume_decimal_digits_and_build_token();
    }

    return token;
}

const long long max_fixnum = std::numeric_limits<long long>::max() / 2; // 63 bits for MRI

Token Lexer::chars_to_fixnum_or_bignum_token(SharedPtr<String> chars, int base, int offset) {
    errno = 0;
    auto fixnum = strtoll(chars->c_str() + offset, nullptr, base);
    if (errno != 0 || fixnum > max_fixnum)
        return Token { Token::Type::Bignum, chars, m_file, m_token_line, m_token_column, m_whitespace_precedes };
    else
        return Token { Token::Type::Fixnum, fixnum, m_file, m_token_line, m_token_column, m_whitespace_precedes };
}

Token Lexer::consume_numeric_as_float(SharedPtr<String> chars) {
    char c = current_char();
    if (c == '.') {
        chars->append_char('.');
        c = next();
        do {
            chars->append_char(c);
            c = next();
            if (c == '_')
                c = next();
        } while (isdigit(c));
    }
    if (c == 'e' || c == 'E') {
        chars->append_char('e');
        c = next();
        if (c == '-' || c == '+') {
            chars->append_char(c);
            c = next();
        }
        if (!isdigit(c))
            return Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column, m_whitespace_precedes };
        do {
            chars->append_char(c);
            c = next();
            if (c == '_')
                c = next();
        } while (isdigit(c));
    }
    double dbl = atof(chars->c_str());
    return Token { Token::Type::Float, dbl, m_file, m_token_line, m_token_column, m_whitespace_precedes };
}

Token Lexer::consume_nth_ref() {
    char c = next();
    long long num = 0;
    do {
        num *= 10;
        num += c - '0';
        c = next();
    } while (isdigit(c));
    return Token { Token::Type::NthRef, num, m_file, m_token_line, m_token_column, m_whitespace_precedes };
}

long long Lexer::consume_hex_number(int max_length, bool allow_underscore) {
    char c = current_char();
    int length = 0;
    long long number = 0;
    do {
        number *= 16;
        if (c >= 'a' && c <= 'f')
            number += c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            number += c - 'A' + 10;
        else
            number += c - '0';
        c = next();
        if (allow_underscore && c == '_')
            c = next();
    } while (isxdigit(c) && (max_length == 0 || ++length < max_length));
    return number;
}

long long Lexer::consume_octal_number(int max_length, bool allow_underscore) {
    char c = current_char();
    int length = 0;
    long long number = 0;
    do {
        number *= 8;
        number += c - '0';
        c = next();
        if (allow_underscore && c == '_')
            c = next();
    } while (c >= '0' && c <= '7' && (max_length == 0 || ++length < max_length));
    return number;
}

// public domain
// https://gist.github.com/Miouyouyou/864130e8734afe3f806512b14022226f
void Lexer::utf32_codepoint_to_utf8(String &buf, long long codepoint) {
    if (codepoint < 0x80) {
        buf.append_char(codepoint);
    } else if (codepoint < 0x800) { // 00000yyy yyxxxxxx
        buf.append_char(0b11000000 | (codepoint >> 6));
        buf.append_char(0b10000000 | (codepoint & 0x3f));
    } else if (codepoint < 0x10000) { // zzzzyyyy yyxxxxxx
        buf.append_char(0b11100000 | (codepoint >> 12));
        buf.append_char(0b10000000 | ((codepoint >> 6) & 0x3f));
        buf.append_char(0b10000000 | (codepoint & 0x3f));
    } else if (codepoint < 0x200000) { // 000uuuuu zzzzyyyy yyxxxxxx
        buf.append_char(0b11110000 | (codepoint >> 18));
        buf.append_char(0b10000000 | ((codepoint >> 12) & 0x3f));
        buf.append_char(0b10000000 | ((codepoint >> 6) & 0x3f));
        buf.append_char(0b10000000 | (codepoint & 0x3f));
    } else {
        TM_UNREACHABLE();
    }
}

std::pair<bool, Token::Type> Lexer::consume_escaped_byte(String &buf) {
    auto control_character = [&](bool meta) {
        char c = next();
        if (c == '-')
            c = next();
        int num = 0;
        if (!meta && c == '\\' && peek() == 'M') {
            advance(); // M
            c = next();
            if (c != '-')
                return -1;
            meta = true;
            c = next();
        }
        if (c == '?')
            num = 127;
        else if (c >= ' ' && c <= '>')
            num = c - ' ';
        else if (c >= '@' && c <= '_')
            num = c - '@';
        else if (c >= '`' && c <= '~')
            num = c - '`';
        if (meta)
            return num + 128;
        else
            return num;
    };
    auto c = current_char();
    if (c >= '0' && c <= '7') {
        auto number = consume_octal_number(3);
        buf.append_char(number);
    } else if (c == 'x') {
        // hex: 1-2 digits
        advance();
        auto number = consume_hex_number(2);
        buf.append_char(number);
    } else if (c == 'u') {
        c = next();
        if (c == '{') {
            c = next();
            // unicode characters, space separated, 1-6 hex digits
            while (c != '}') {
                if (!isxdigit(c))
                    return { false, Token::Type::InvalidUnicodeEscape };
                auto codepoint = consume_hex_number(6);
                utf32_codepoint_to_utf8(buf, codepoint);
                c = current_char();
                while (c == ' ')
                    c = next();
            }
            if (c == '}')
                advance();
        } else {
            // unicode: 4 hex digits
            auto codepoint = consume_hex_number(4);
            utf32_codepoint_to_utf8(buf, codepoint);
        }
    } else {
        switch (c) {
        case 'a':
            buf.append_char('\a');
            break;
        case 'b':
            buf.append_char('\b');
            break;
        case 'c':
        case 'C': {
            int num = control_character(false);
            if (num == -1)
                return { false, Token::Type::InvalidCharacterEscape };
            buf.append_char((unsigned char)num);
            break;
        }
        case 'e':
            buf.append_char('\e');
            break;
        case 'f':
            buf.append_char('\f');
            break;
        case 'M': {
            c = next();
            if (c != '-')
                return { false, Token::Type::InvalidCharacterEscape };
            c = next();
            int num = 0;
            if (c == '\\' && (peek() == 'c' || peek() == 'C')) {
                advance();
                num = control_character(true);
            } else {
                num = (int)c + 128;
            }
            buf.append_char((unsigned char)num);
            break;
        }
        case 'n':
            buf.append_char('\n');
            break;
        case 'r':
            buf.append_char('\r');
            break;
        case 's':
            buf.append_char((unsigned char)32);
            break;
        case 't':
            buf.append_char('\t');
            break;
        case 'v':
            buf.append_char('\v');
            break;
        case '\n':
            break;
        default:
            buf.append_char(c);
            break;
        }
        advance();
    }
    return { true, Token::Type::String };
}

bool Lexer::token_is_first_on_line() const {
    return !m_last_token || m_last_token.is_newline();
}

Token Lexer::consume_double_quoted_string(char start_char, char stop_char, Token::Type begin_type, Token::Type end_type) {
    m_nested_lexer = new InterpolatedStringLexer { *this, start_char, stop_char, end_type };
    return Token { begin_type, start_char, m_file, m_token_line, m_token_column, m_whitespace_precedes };
}

Token Lexer::consume_single_quoted_string(char start_char, char stop_char) {
    int pair_depth = 0;
    SharedPtr<String> buf = new String("");
    char c = current_char();
    while (c) {
        if (c == '\\') {
            c = next();
            if (c == stop_char || c == '\\') {
                buf->append_char(c);
            } else {
                buf->append_char('\\');
                buf->append_char(c);
            }
        } else if (c == start_char && start_char != stop_char) {
            pair_depth++;
            buf->append_char(c);
        } else if (c == stop_char) {
            if (pair_depth > 0) {
                pair_depth--;
                buf->append_char(c);
            } else {
                advance(); // '
                if (current_char() == ':' && !m_open_ternary) {
                    advance(); // :
                    return Token { Token::Type::SymbolKey, buf, m_file, m_token_line, m_token_column, m_whitespace_precedes };
                } else {
                    return Token { Token::Type::String, buf, m_file, m_token_line, m_token_column, m_whitespace_precedes };
                }
            }
        } else {
            buf->append_char(c);
        }
        c = next();
    }
    return Token { Token::Type::UnterminatedString, start_char, m_file, m_token_line, m_token_column, m_whitespace_precedes };
}

Token Lexer::consume_quoted_array_without_interpolation(char start_char, char stop_char, Token::Type type) {
    m_nested_lexer = new WordArrayLexer { *this, start_char, stop_char, false };
    return Token { type, start_char, m_file, m_token_line, m_token_column, m_whitespace_precedes };
}

Token Lexer::consume_quoted_array_with_interpolation(char start_char, char stop_char, Token::Type type) {
    m_nested_lexer = new WordArrayLexer { *this, start_char, stop_char, true };
    return Token { type, start_char, m_file, m_token_line, m_token_column, m_whitespace_precedes };
}

Token Lexer::consume_regexp(char start_char, char stop_char) {
    m_nested_lexer = new RegexpLexer { *this, start_char, stop_char };
    return Token { Token::Type::InterpolatedRegexpBegin, start_char, m_file, m_token_line, m_token_column, m_whitespace_precedes };
}

SharedPtr<String> Lexer::consume_non_whitespace() {
    char c = current_char();
    SharedPtr<String> buf = new String("");
    do {
        buf->append_char(c);
        c = next();
    } while (c && c != ' ' && c != '\t' && c != '\n' && c != '\r');
    return buf;
}
};
