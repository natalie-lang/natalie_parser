#include "natalie_parser/lexer/regexp_lexer.hpp"
#include "natalie_parser/token.hpp"

namespace NatalieParser {

Token RegexpLexer::build_next_token() {
    switch (m_state) {
    case State::InProgress:
        return consume_regexp();
    case State::EvaluateBegin:
        m_nested_lexer = new Lexer { *this };
        m_nested_lexer->set_stop_char('}');
        m_state = State::EvaluateEnd;
        return Token { Token::Type::EvaluateToStringBegin, m_file, m_token_line, m_token_column };
    case State::EvaluateEnd:
        advance(); // }
        if (current_char() == m_stop_char) {
            advance();
            m_options = consume_options();
            m_state = State::EndToken;
        } else {
            m_state = State::InProgress;
        }
        return Token { Token::Type::EvaluateToStringEnd, m_file, m_token_line, m_token_column };
    case State::EndToken: {
        m_state = State::Done;
        auto token = Token { Token::Type::InterpolatedRegexpEnd, m_file, m_cursor_line, m_cursor_column };
        if (m_options && !m_options->is_empty())
            token.set_literal(m_options);
        return token;
    }
    case State::Done:
        return Token { Token::Type::Eof, m_file, m_cursor_line, m_cursor_column };
    }
    TM_UNREACHABLE();
}

Token RegexpLexer::consume_regexp() {
    SharedPtr<String> buf = new String;
    while (auto c = current_char()) {
        if (c == '\\') {
            c = next();
            switch (c) {
            case '/':
                buf->append_char(c);
                break;
            default:
                if (c == m_stop_char) {
                    buf->append_char(c);
                } else {
                    buf->append_char('\\');
                    buf->append_char(c);
                }
                break;
            }
            advance();
        } else if (c == '#' && peek() == '{') {
            auto token = Token { Token::Type::String, buf, m_file, m_token_line, m_token_column };
            buf = new String;
            advance(2);
            m_state = State::EvaluateBegin;
            return token;
        } else if (c == m_start_char && m_start_char != m_stop_char) {
            m_pair_depth++;
            advance();
            buf->append_char(c);
        } else if (c == m_stop_char) {
            advance();
            if (m_pair_depth > 0) {
                m_pair_depth--;
                buf->append_char(c);
            } else {
                m_options = consume_options();
                m_state = State::EndToken;
                return Token { Token::Type::String, buf, m_file, m_token_line, m_token_column };
            }
        } else {
            buf->append_char(c);
            advance();
        }
    }
    return Token { Token::Type::UnterminatedRegexp, buf, m_file, m_token_line, m_token_column };
}

String *RegexpLexer::consume_options() {
    char c = current_char();
    auto options = new String;
    while (c == 'i' || c == 'm' || c == 'x' || c == 'o' || c == 'u' || c == 'e' || c == 's' || c == 'n') {
        options->append_char(c);
        c = next();
    }
    return options;
}

};
