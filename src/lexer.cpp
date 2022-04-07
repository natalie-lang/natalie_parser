#include <errno.h>
#include <limits>
#include <stdlib.h>

#include "natalie_parser/lexer.hpp"
#include "natalie_parser/lexer/interpolated_string_lexer.hpp"
#include "natalie_parser/lexer/regexp_lexer.hpp"
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
        if (skip_next_newline && token.is_newline())
            continue;
        if (skip_next_newline && !token.is_newline())
            skip_next_newline = false;

        // get rid of newlines before certain tokens
        while (token.can_follow_collapsible_newline() && !tokens->is_empty() && tokens->last().is_newline())
            tokens->pop();

        // convert semicolons to eol tokens
        if (token.is_semicolon())
            token = Token { Token::Type::Eol, token.file(), token.line(), token.column() };

        if (last_doc_token && token.can_have_doc()) {
            token.set_doc(last_doc_token.literal_string());
            last_doc_token = {};
        }

        tokens->push(token);

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
            if (!m_heredoc_stack.is_empty()) {
                // heredocs work differently
            } else {
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
    auto token = build_next_token();
    m_last_token = token;
    return token;
}

bool is_identifier_char(char c) {
    if (!c) return false;
    return isalnum(c) || c == '_';
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
    while (current_char() == ' ' || current_char() == '\t') {
        whitespace_found = true;
        advance();
    }
    return whitespace_found;
}

Token Lexer::build_next_token() {
    if (m_index >= m_size)
        return Token { Token::Type::Eof, m_file, m_cursor_line, m_cursor_column };
    if (m_stop_char && current_char() == m_stop_char)
        return Token { Token::Type::Eof, m_file, m_cursor_line, m_cursor_column };
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
                return Token { Token::Type::EqualEqualEqual, m_file, m_token_line, m_token_column };
            }
            default:
                return Token { Token::Type::EqualEqual, m_file, m_token_line, m_token_column };
            }
        }
        case '>':
            advance();
            return Token { Token::Type::HashRocket, m_file, m_token_line, m_token_column };
        case '~':
            advance();
            return Token { Token::Type::Match, m_file, m_token_line, m_token_column };
        default:
            if (m_cursor_column == 1 && match(5, "begin")) {
                SharedPtr<String> doc = new String("=begin");
                char c = current_char();
                do {
                    doc->append_char(c);
                    c = next();
                } while (!(m_cursor_column == 0 && match(4, "=end")));
                doc->append("=end\n");
                return Token { Token::Type::Doc, doc, m_file, m_token_line, m_token_column };
            }
            return Token { Token::Type::Equal, m_file, m_token_line, m_token_column };
        }
    }
    case '+':
        advance();
        switch (current_char()) {
        case '=':
            advance();
            return Token { Token::Type::PlusEqual, m_file, m_token_line, m_token_column };
        case '@':
            if (m_last_token.is_def_keyword() || m_last_token.is_dot()) {
                advance();
                SharedPtr<String> lit = new String("+@");
                return Token { Token::Type::BareName, lit, m_file, m_token_line, m_token_column };
            } else {
                return Token { Token::Type::Plus, m_file, m_token_line, m_token_column };
            }
        default:
            return Token { Token::Type::Plus, m_file, m_token_line, m_token_column };
        }
    case '-':
        advance();
        switch (current_char()) {
        case '>':
            advance();
            return Token { Token::Type::Arrow, m_file, m_token_line, m_token_column };
        case '=':
            advance();
            return Token { Token::Type::MinusEqual, m_file, m_token_line, m_token_column };
        case '@':
            if (m_last_token.is_def_keyword() || m_last_token.is_dot()) {
                advance();
                SharedPtr<String> lit = new String("-@");
                return Token { Token::Type::BareName, lit, m_file, m_token_line, m_token_column };
            } else {
                return Token { Token::Type::Minus, m_file, m_token_line, m_token_column };
            }
        default:
            return Token { Token::Type::Minus, m_file, m_token_line, m_token_column };
        }
    case '*':
        advance();
        switch (current_char()) {
        case '*':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::ExponentEqual, m_file, m_token_line, m_token_column };
            default:
                return Token { Token::Type::Exponent, m_file, m_token_line, m_token_column };
            }
        case '=':
            advance();
            return Token { Token::Type::MultiplyEqual, m_file, m_token_line, m_token_column };
        default:
            return Token { Token::Type::Multiply, m_file, m_token_line, m_token_column };
        }
    case '/': {
        advance();
        if (!m_last_token)
            return consume_regexp('/');
        switch (m_last_token.type()) {
        case Token::Type::Comma:
        case Token::Type::LBracket:
        case Token::Type::LCurlyBrace:
        case Token::Type::LParen:
        case Token::Type::Match:
            return consume_regexp('/');
        case Token::Type::DefKeyword:
            return Token { Token::Type::Divide, m_file, m_token_line, m_token_column };
        default: {
            switch (current_char()) {
            case ' ':
                return Token { Token::Type::Divide, m_file, m_token_line, m_token_column };
            case '=':
                advance();
                return Token { Token::Type::DivideEqual, m_file, m_token_line, m_token_column };
            default:
                if (m_whitespace_precedes) {
                    return consume_regexp('/');
                } else {
                    return Token { Token::Type::Divide, m_file, m_token_line, m_token_column };
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
            return Token { Token::Type::ModulusEqual, m_file, m_token_line, m_token_column };
        case 'q':
            switch (peek()) {
            case '/':
                advance(2);
                return consume_single_quoted_string('/');
            case '[':
                advance(2);
                return consume_single_quoted_string(']');
            case '{':
                advance(2);
                return consume_single_quoted_string('}');
            case '(':
                advance(2);
                return consume_single_quoted_string(')');
            default:
                return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
            }
        case 'Q':
            switch (peek()) {
            case '/':
                advance(2);
                return consume_double_quoted_string('/');
            case '[':
                advance(2);
                return consume_double_quoted_string(']');
            case '{':
                advance(2);
                return consume_double_quoted_string('}');
            case '(':
                advance(2);
                return consume_double_quoted_string(')');
            default:
                return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
            }
        case 'r':
            switch (peek()) {
            case '/':
            case '|': {
                char c = next();
                advance();
                return consume_regexp(c);
            }
            case '[':
                advance(2);
                return consume_regexp(']');
            case '{':
                advance(2);
                return consume_regexp('}');
            case '(':
                advance(2);
                return consume_regexp(')');
            default:
                return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
            }
        case 'x':
            switch (peek()) {
            case '/': {
                advance(2);
                return consume_double_quoted_string('/', Token::Type::InterpolatedShellBegin, Token::Type::InterpolatedShellEnd);
            }
            case '[': {
                advance(2);
                return consume_double_quoted_string(']', Token::Type::InterpolatedShellBegin, Token::Type::InterpolatedShellEnd);
            }
            case '{': {
                advance(2);
                return consume_double_quoted_string('}', Token::Type::InterpolatedShellBegin, Token::Type::InterpolatedShellEnd);
            }
            case '(': {
                advance(2);
                return consume_double_quoted_string(')', Token::Type::InterpolatedShellBegin, Token::Type::InterpolatedShellEnd);
            }
            default:
                return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
            }
        case 'w':
            switch (peek()) {
            case '/':
            case '|': {
                char c = next();
                advance();
                return consume_quoted_array_without_interpolation(c, Token::Type::PercentLowerW);
            }
            case '[':
                advance(2);
                return consume_quoted_array_without_interpolation(']', Token::Type::PercentLowerW);
            case '{':
                advance(2);
                return consume_quoted_array_without_interpolation('}', Token::Type::PercentLowerW);
            case '(':
                advance(2);
                return consume_quoted_array_without_interpolation(')', Token::Type::PercentLowerW);
            default:
                return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
            }
        case 'W':
            switch (peek()) {
            case '/':
            case '|': {
                char c = next();
                advance();
                return consume_quoted_array_without_interpolation(c, Token::Type::PercentUpperW);
            }
            case '[':
                advance(2);
                return consume_quoted_array_with_interpolation(']', Token::Type::PercentUpperW);
            case '{':
                advance(2);
                return consume_quoted_array_with_interpolation('}', Token::Type::PercentUpperW);
            case '(':
                advance(2);
                return consume_quoted_array_with_interpolation(')', Token::Type::PercentUpperW);
            default:
                return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
            }
        case 'i':
            switch (peek()) {
            case '/':
                advance(2);
                return consume_quoted_array_without_interpolation('/', Token::Type::PercentLowerI);
            case '[':
                advance(2);
                return consume_quoted_array_without_interpolation(']', Token::Type::PercentLowerI);
            case '{':
                advance(2);
                return consume_quoted_array_without_interpolation('}', Token::Type::PercentLowerI);
            case '(':
                advance(2);
                return consume_quoted_array_without_interpolation(')', Token::Type::PercentLowerI);
            default:
                return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
            }
        case 'I':
            switch (peek()) {
            case '/':
                advance(2);
                return consume_quoted_array_with_interpolation('/', Token::Type::PercentUpperI);
            case '[':
                advance(2);
                return consume_quoted_array_with_interpolation(']', Token::Type::PercentUpperI);
            case '{':
                advance(2);
                return consume_quoted_array_with_interpolation('}', Token::Type::PercentUpperI);
            case '(':
                advance(2);
                return consume_quoted_array_with_interpolation(')', Token::Type::PercentUpperI);
            default:
                return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
            }
        case '/':
        case '|': {
            char c = current_char();
            advance();
            return consume_single_quoted_string(c);
        }
        case '[':
            advance();
            return consume_single_quoted_string(']');
        case '{':
            advance();
            return consume_single_quoted_string('}');
        case '(':
            if (m_last_token.type() == Token::Type::DefKeyword || m_last_token.type() == Token::Type::Dot) {
                // It's a trap! This looks like a %(string) but it's a method def/call!
                break;
            }
            advance();
            return consume_single_quoted_string(')');
        }
        return Token { Token::Type::Modulus, m_file, m_token_line, m_token_column };
    case '!':
        advance();
        switch (current_char()) {
        case '=':
            advance();
            return Token { Token::Type::NotEqual, m_file, m_token_line, m_token_column };
        case '~':
            advance();
            return Token { Token::Type::NotMatch, m_file, m_token_line, m_token_column };
        default:
            return Token { Token::Type::Not, m_file, m_token_line, m_token_column };
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
                    return Token { Token::Type::LeftShift, m_file, m_token_line, m_token_column };
                }
            }
            case '=':
                advance();
                return Token { Token::Type::LeftShiftEqual, m_file, m_token_line, m_token_column };
            default:
                if (!m_whitespace_precedes) {
                    if (m_last_token.is_eol() || m_index == 2) // start of line or start of file
                        return consume_heredoc();
                    else if (m_last_token.type() == Token::Type::Equal)
                        return consume_heredoc();
                    else if (m_last_token.is_operator())
                        return consume_heredoc();
                    else
                        return Token { Token::Type::LeftShift, m_file, m_token_line, m_token_column };
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
                    return Token { Token::Type::LeftShift, m_file, m_token_line, m_token_column };
                }
            }
        }
        case '=':
            advance();
            switch (current_char()) {
            case '>':
                advance();
                return Token { Token::Type::Comparison, m_file, m_token_line, m_token_column };
            default:
                return Token { Token::Type::LessThanOrEqual, m_file, m_token_line, m_token_column };
            }
        default:
            return Token { Token::Type::LessThan, m_file, m_token_line, m_token_column };
        }
    case '>':
        advance();
        switch (current_char()) {
        case '>':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::RightShiftEqual, m_file, m_token_line, m_token_column };
            default:
                return Token { Token::Type::RightShift, m_file, m_token_line, m_token_column };
            }
        case '=':
            advance();
            return Token { Token::Type::GreaterThanOrEqual, m_file, m_token_line, m_token_column };
        default:
            return Token { Token::Type::GreaterThan, m_file, m_token_line, m_token_column };
        }
    case '&':
        advance();
        switch (current_char()) {
        case '&':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::AndEqual, m_file, m_token_line, m_token_column };
            default:
                return Token { Token::Type::And, m_file, m_token_line, m_token_column };
            }
        case '=':
            advance();
            return Token { Token::Type::BitwiseAndEqual, m_file, m_token_line, m_token_column };
        case '.':
            advance();
            return Token { Token::Type::SafeNavigation, m_file, m_token_line, m_token_column };
        default:
            return Token { Token::Type::BitwiseAnd, m_file, m_token_line, m_token_column };
        }
    case '|':
        advance();
        switch (current_char()) {
        case '|':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::OrEqual, m_file, m_token_line, m_token_column };
            default:
                return Token { Token::Type::Or, m_file, m_token_line, m_token_column };
            }
        case '=':
            advance();
            return Token { Token::Type::BitwiseOrEqual, m_file, m_token_line, m_token_column };
        default:
            return Token { Token::Type::BitwiseOr, m_file, m_token_line, m_token_column };
        }
    case '^':
        advance();
        switch (current_char()) {
        case '=':
            advance();
            return Token { Token::Type::BitwiseXorEqual, m_file, m_token_line, m_token_column };
        default:
            return Token { Token::Type::BitwiseXor, m_file, m_token_line, m_token_column };
        }
    case '~':
        advance();
        return Token { Token::Type::Tilde, m_file, m_token_line, m_token_column };
    case '?':
        advance();
        return Token { Token::Type::TernaryQuestion, m_file, m_token_line, m_token_column };
    case ':': {
        auto c = next();
        if (c == ':') {
            advance();
            return Token { Token::Type::ConstantResolution, m_file, m_token_line, m_token_column };
        } else if (c == '"') {
            advance();
            return consume_double_quoted_string('"', Token::Type::InterpolatedSymbolBegin, Token::Type::InterpolatedSymbolEnd);
        } else if (c == '\'') {
            advance();
            auto string = consume_single_quoted_string('\'');
            return Token { Token::Type::Symbol, string.literal(), m_file, m_token_line, m_token_column };
        } else if (isspace(c)) {
            return Token { Token::Type::TernaryColon, m_file, m_token_line, m_token_column };
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
            return Token { Token::Type::BackRef, '&', m_file, m_token_line, m_token_column };
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
                return Token { Token::Type::DotDotDot, m_file, m_token_line, m_token_column };
            default:
                return Token { Token::Type::DotDot, m_file, m_token_line, m_token_column };
            }
        default:
            return Token { Token::Type::Dot, m_file, m_token_line, m_token_column };
        }
    case '{':
        advance();
        return Token { Token::Type::LCurlyBrace, m_file, m_token_line, m_token_column };
    case '[': {
        advance();
        switch (current_char()) {
        case ']':
            advance();
            switch (current_char()) {
            case '=':
                advance();
                return Token { Token::Type::LBracketRBracketEqual, m_file, m_token_line, m_token_column };
            default:
                auto token = Token { Token::Type::LBracketRBracket, m_file, m_token_line, m_token_column };
                token.set_whitespace_precedes(m_whitespace_precedes);
                return token;
            }
        default:
            auto token = Token { Token::Type::LBracket, m_file, m_token_line, m_token_column };
            token.set_whitespace_precedes(m_whitespace_precedes);
            return token;
        }
    }
    case '(':
        advance();
        return Token { Token::Type::LParen, m_file, m_token_line, m_token_column };
    case '}':
        advance();
        return Token { Token::Type::RCurlyBrace, m_file, m_token_line, m_token_column };
    case ']':
        advance();
        return Token { Token::Type::RBracket, m_file, m_token_line, m_token_column };
    case ')':
        advance();
        return Token { Token::Type::RParen, m_file, m_token_line, m_token_column };
    case '\n': {
        advance();
        auto token = Token { Token::Type::Eol, m_file, m_token_line, m_token_column };
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
        return Token { Token::Type::Semicolon, m_file, m_token_line, m_token_column };
    case ',':
        advance();
        return Token { Token::Type::Comma, m_file, m_token_line, m_token_column };
    case '"':
        advance();
        return consume_double_quoted_string('"');
    case '\'':
        advance();
        return consume_single_quoted_string('\'');
    case '`': {
        advance();
        return consume_double_quoted_string('`', Token::Type::InterpolatedShellBegin, Token::Type::InterpolatedShellEnd);
    }
    case '#':
        if (token_is_first_on_line()) {
            SharedPtr<String> doc = new String("#");
            char c;
            do {
                c = next();
                doc->append_char(c);
            } while (c && c != '\n' && c != '\r');
            return Token { Token::Type::Doc, doc, m_file, m_token_line, m_token_column };
        } else {
            char c;
            do {
                c = next();
            } while (c && c != '\n' && c != '\r');
            return Token { Token::Type::Comment, m_file, m_token_line, m_token_column };
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
        if (isalpha(current_char())) {
            return Token { Token::Type::Invalid, current_char(), m_file, m_cursor_line, m_cursor_column };
        }
        return token;
    }
    };
    if (!token) {
        if (match(12, "__ENCODING__"))
            return Token { Token::Type::ENCODINGKeyword, m_file, m_token_line, m_token_column };
        else if (match(8, "__LINE__"))
            return Token { Token::Type::LINEKeyword, m_file, m_token_line, m_token_column };
        else if (match(8, "__FILE__"))
            return Token { Token::Type::FILEKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "BEGIN"))
            return Token { Token::Type::BEGINKeyword, m_file, m_token_line, m_token_column };
        else if (match(3, "END"))
            return Token { Token::Type::ENDKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "alias"))
            return Token { Token::Type::AliasKeyword, m_file, m_token_line, m_token_column };
        else if (match(3, "and"))
            return Token { Token::Type::AndKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "begin"))
            return Token { Token::Type::BeginKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "break"))
            return Token { Token::Type::BreakKeyword, m_file, m_token_line, m_token_column };
        else if (match(4, "case"))
            return Token { Token::Type::CaseKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "class"))
            return Token { Token::Type::ClassKeyword, m_file, m_token_line, m_token_column };
        else if (match(8, "defined?"))
            return Token { Token::Type::DefinedKeyword, m_file, m_token_line, m_token_column };
        else if (match(3, "def"))
            return Token { Token::Type::DefKeyword, m_file, m_token_line, m_token_column };
        else if (match(2, "do"))
            return Token { Token::Type::DoKeyword, m_file, m_token_line, m_token_column };
        else if (match(4, "else"))
            return Token { Token::Type::ElseKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "elsif"))
            return Token { Token::Type::ElsifKeyword, m_file, m_token_line, m_token_column };
        else if (match(3, "end"))
            return Token { Token::Type::EndKeyword, m_file, m_token_line, m_token_column };
        else if (match(6, "ensure"))
            return Token { Token::Type::EnsureKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "false"))
            return Token { Token::Type::FalseKeyword, m_file, m_token_line, m_token_column };
        else if (match(3, "for"))
            return Token { Token::Type::ForKeyword, m_file, m_token_line, m_token_column };
        else if (match(2, "if"))
            return Token { Token::Type::IfKeyword, m_file, m_token_line, m_token_column };
        else if (match(2, "in"))
            return Token { Token::Type::InKeyword, m_file, m_token_line, m_token_column };
        else if (match(6, "module"))
            return Token { Token::Type::ModuleKeyword, m_file, m_token_line, m_token_column };
        else if (match(4, "next"))
            return Token { Token::Type::NextKeyword, m_file, m_token_line, m_token_column };
        else if (match(3, "nil"))
            return Token { Token::Type::NilKeyword, m_file, m_token_line, m_token_column };
        else if (match(3, "not"))
            return Token { Token::Type::NotKeyword, m_file, m_token_line, m_token_column };
        else if (match(2, "or"))
            return Token { Token::Type::OrKeyword, m_file, m_token_line, m_token_column };
        else if (match(4, "redo"))
            return Token { Token::Type::RedoKeyword, m_file, m_token_line, m_token_column };
        else if (match(6, "rescue"))
            return Token { Token::Type::RescueKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "retry"))
            return Token { Token::Type::RetryKeyword, m_file, m_token_line, m_token_column };
        else if (match(6, "return"))
            return Token { Token::Type::ReturnKeyword, m_file, m_token_line, m_token_column };
        else if (match(4, "self"))
            return Token { Token::Type::SelfKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "super"))
            return Token { Token::Type::SuperKeyword, m_file, m_token_line, m_token_column };
        else if (match(4, "then"))
            return Token { Token::Type::ThenKeyword, m_file, m_token_line, m_token_column };
        else if (match(4, "true"))
            return Token { Token::Type::TrueKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "undef"))
            return Token { Token::Type::UndefKeyword, m_file, m_token_line, m_token_column };
        else if (match(6, "unless"))
            return Token { Token::Type::UnlessKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "until"))
            return Token { Token::Type::UntilKeyword, m_file, m_token_line, m_token_column };
        else if (match(4, "when"))
            return Token { Token::Type::WhenKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "while"))
            return Token { Token::Type::WhileKeyword, m_file, m_token_line, m_token_column };
        else if (match(5, "yield"))
            return Token { Token::Type::YieldKeyword, m_file, m_token_line, m_token_column };
        auto c = current_char();
        if ((c >= 'a' && c <= 'z') || c == '_') {
            return consume_bare_name();
        } else if (c >= 'A' && c <= 'Z') {
            return consume_constant();
        } else {
            auto buf = consume_non_whitespace();
            auto token = Token { Token::Type::Invalid, buf, m_file, m_token_line, m_token_column };
            return token;
        }
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
            return Token { Token::Type::Invalid, c, m_file, m_token_line, m_token_column };
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
            return Token { Token::Type::TernaryColon, m_file, m_token_line, m_token_column };
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
            gobble(c);
        default:
            break;
        }
    }
    return Token { Token::Type::Symbol, buf, m_file, m_token_line, m_token_column };
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
    return Token { type, buf, m_file, m_token_line, m_token_column };
}

Token Lexer::consume_bare_name() {
    auto token = consume_word(Token::Type::BareName);
    auto c = current_char();
    if (c == ':' && peek() != ':') {
        advance();
        token.set_type(Token::Type::SymbolKey);
    }
    return token;
}

Token Lexer::consume_constant() {
    auto token = consume_word(Token::Type::Constant);
    auto c = current_char();
    if (c == ':' && peek() != ':') {
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
        return Token { Token::Type::GlobalVariable, buf, m_file, m_token_line, m_token_column };
    }
    case '-': {
        SharedPtr<String> buf = new String("$-");
        advance(2);
        buf->append_char(current_char());
        advance();
        return Token { Token::Type::GlobalVariable, buf, m_file, m_token_line, m_token_column };
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
        heredoc_name = String(consume_word(Token::Type::BareName).literal());
    }

    if (delimiter) {
        char c = next();
        while (c != delimiter) {
            switch (c) {
            case '\n':
            case '\r':
            case 0:
                return Token { Token::Type::UnterminatedString, "heredoc identifier", m_file, m_token_line, m_token_column };
            default:
                heredoc_name.append_char(c);
                c = next();
            }
        }
        advance();
    }

    SharedPtr<String> doc = new String("");
    size_t heredoc_index = m_index;
    auto get_char = [&heredoc_index, this]() { return (heredoc_index >= m_size) ? 0 : m_input->at(heredoc_index); };

    if (m_heredoc_stack.is_empty()) {
        // start consuming the heredoc on the next line
        while (get_char() != '\n') {
            if (heredoc_index >= m_size)
                return Token { Token::Type::UnterminatedString, "heredoc", m_file, m_token_line, m_token_column };
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
            return Token { Token::Type::UnterminatedString, doc, m_file, m_token_line, m_token_column };
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

    // we have to keep tokenizing on the line where the heredoc was started, and then jump to the line after the heredoc
    // this index is used to do that
    m_heredoc_stack.push(heredoc_index);

    auto token = Token { Token::Type::String, doc, m_file, m_token_line, m_token_column };

    if (should_interpolate) {
        m_nested_lexer = new InterpolatedStringLexer { *this, token, end_type };
        return Token { begin_type, m_file, m_token_line, m_token_column };
    }

    return token;
}

Token Lexer::consume_numeric() {
    SharedPtr<String> chars = new String;
    if (current_char() == '0') {
        switch (peek()) {
        case 'd':
        case 'D': {
            advance();
            char c = next();
            if (!isdigit(c))
                return Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column };
            do {
                chars->append_char(c);
                c = next();
                if (c == '_')
                    c = next();
            } while (isdigit(c));
            return chars_to_fixnum_or_bignum_token(chars, 10, 0);
        }
        case 'o':
        case 'O': {
            chars->append_char('0');
            chars->append_char('o');
            advance();
            char c = next();
            if (!(c >= '0' && c <= '7'))
                return Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column };
            do {
                chars->append_char(c);
                c = next();
                if (c == '_')
                    c = next();
            } while (c >= '0' && c <= '7');
            return chars_to_fixnum_or_bignum_token(chars, 8, 2);
        }
        case 'x':
        case 'X': {
            chars->append_char('0');
            chars->append_char('x');
            advance();
            char c = next();
            if (!isxdigit(c))
                return Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column };
            do {
                chars->append_char(c);
                c = next();
                if (c == '_')
                    c = next();
            } while (isxdigit(c));
            return chars_to_fixnum_or_bignum_token(chars, 16, 2);
        }
        case 'b':
        case 'B': {
            chars->append_char('0');
            chars->append_char('b');
            advance();
            char c = next();
            if (c != '0' && c != '1')
                return Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column };
            do {
                chars->append_char(c);
                c = next();
                if (c == '_')
                    c = next();
            } while (c == '0' || c == '1');
            return chars_to_fixnum_or_bignum_token(chars, 2, 2);
        }
        }
    }
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
}

const long long max_fixnum = std::numeric_limits<long long>::max() / 2; // 63 bits for MRI

Token Lexer::chars_to_fixnum_or_bignum_token(SharedPtr<String> chars, int base, int offset) {
    errno = 0;
    auto fixnum = strtoll(chars->c_str() + offset, nullptr, base);
    if (errno != 0 || fixnum > max_fixnum)
        return Token { Token::Type::Bignum, chars, m_file, m_token_line, m_token_column };
    else
        return Token { Token::Type::Fixnum, fixnum, m_file, m_token_line, m_token_column };
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
            return Token { Token::Type::Invalid, c, m_file, m_cursor_line, m_cursor_column };
        do {
            chars->append_char(c);
            c = next();
            if (c == '_')
                c = next();
        } while (isdigit(c));
    }
    double dbl = atof(chars->c_str());
    return Token { Token::Type::Float, dbl, m_file, m_token_line, m_token_column };
}

Token Lexer::consume_nth_ref() {
    char c = next();
    long long num = 0;
    do {
        num *= 10;
        num += c - '0';
        c = next();
    } while (isdigit(c));
    return Token { Token::Type::NthRef, num, m_file, m_token_line, m_token_column };
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

bool Lexer::token_is_first_on_line() const {
    return !m_last_token || m_last_token.is_eol();
}

Token Lexer::consume_double_quoted_string(char delimiter, Token::Type begin_type, Token::Type end_type) {
    m_nested_lexer = new InterpolatedStringLexer { *this, delimiter, end_type };
    return Token { begin_type, m_file, m_token_line, m_token_column };
}

Token Lexer::consume_single_quoted_string(char delimiter) {
    SharedPtr<String> buf = new String("");
    char c = current_char();
    while (c) {
        if (c == '\\') {
            c = next();
            switch (c) {
            case '\\':
            case '\'':
                buf->append_char(c);
                break;
            default:
                buf->append_char('\\');
                buf->append_char(c);
                break;
            }
        } else if (c == delimiter) {
            advance();
            return Token { Token::Type::String, buf, m_file, m_token_line, m_token_column };
        } else {
            buf->append_char(c);
        }
        c = next();
    }
    return Token { Token::Type::UnterminatedString, buf, m_file, m_token_line, m_token_column };
}

Token Lexer::consume_quoted_array_without_interpolation(char delimiter, Token::Type type) {
    SharedPtr<String> buf = new String("");
    char c = current_char();
    bool seen_space = false;
    bool seen_start = false;
    for (;;) {
        if (!c) return Token { Token::Type::UnterminatedString, buf, m_file, m_token_line, m_token_column };
        if (c == delimiter) {
            advance();
            break;
        }
        switch (c) {
        case ' ':
        case '\t':
        case '\n':
            if (!seen_space && seen_start) buf->append_char(' ');
            seen_space = true;
            break;
        default:
            buf->append_char(c);
            seen_space = false;
            seen_start = true;
        }
        c = next();
    }
    if (!buf->is_empty() && buf->last_char() == ' ') buf->chomp();
    return Token { type, buf, m_file, m_token_line, m_token_column };
}

Token Lexer::consume_quoted_array_with_interpolation(char delimiter, Token::Type type) {
    SharedPtr<String> buf = new String("");
    char c = current_char();
    bool seen_space = false;
    bool seen_start = false;
    for (;;) {
        if (!c) return Token { Token::Type::UnterminatedString, buf, m_file, m_token_line, m_token_column };
        if (c == delimiter) {
            advance();
            break;
        }
        switch (c) {
        case ' ':
        case '\t':
        case '\n':
            if (!seen_space && seen_start) buf->append_char(' ');
            seen_space = true;
            break;
        default:
            buf->append_char(c);
            seen_space = false;
            seen_start = true;
        }
        c = next();
    }
    if (!buf->is_empty() && buf->last_char() == ' ') buf->chomp();
    return Token { type, buf, m_file, m_token_line, m_token_column };
}

Token Lexer::consume_regexp(char delimiter) {
    m_nested_lexer = new RegexpLexer { *this, delimiter };
    return Token { Token::Type::InterpolatedRegexpBegin, m_file, m_token_line, m_token_column };
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
