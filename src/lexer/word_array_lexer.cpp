#include "natalie_parser/lexer/word_array_lexer.hpp"
#include "natalie_parser/token.hpp"

namespace NatalieParser {

Token WordArrayLexer::build_next_token() {
    switch (m_state) {
    case State::InProgress:
        return consume_array();
    case State::DynamicStringBegin:
        m_state = State::EvaluateBegin;
        return Token { Token::Type::String, m_buffer, m_file, m_token_line, m_token_column };
    case State::DynamicStringEnd:
        if (current_char() == m_stop_char) {
            advance();
            m_state = State::EndToken;
        } else {
            m_state = State::InProgress;
        }
        return Token { Token::Type::InterpolatedStringEnd, m_file, m_token_line, m_token_column };
    case State::EvaluateBegin:
        return start_evaluation();
    case State::EvaluateEnd:
        advance(); // }
        m_state = State::DynamicStringEnd;
        return Token { Token::Type::EvaluateToStringEnd, m_file, m_token_line, m_token_column };
    case State::EndToken:
        m_state = State::Done;
        return Token { Token::Type::RBracket, m_file, m_cursor_line, m_cursor_column };
    case State::Done:
        return Token { Token::Type::Eof, m_file, m_cursor_line, m_cursor_column };
    }
    TM_UNREACHABLE();
}

Token WordArrayLexer::consume_array() {
    m_buffer = new String;
    while (auto c = current_char()) {
        if (c == '\\') {
            c = next();
            advance();
            if (c == ' ') {
                m_buffer->append_char(c);
            } else if (m_interpolated) {
                // FIXME: need to use logic from InterpolatedStringLexer
                switch (c) {
                case 'n':
                    m_buffer->append_char('\n');
                    break;
                case 't':
                    m_buffer->append_char('\t');
                    break;
                default:
                    m_buffer->append_char(c);
                    break;
                }
            } else {
                if (isspace(c)) {
                    m_buffer->append_char(c);
                } else {
                    m_buffer->append_char('\\');
                    m_buffer->append_char(c);
                }
            }
        } else if (isspace(c)) {
            if (!m_buffer->is_empty()) {
                advance();
                return Token { Token::Type::String, m_buffer, m_file, m_token_line, m_token_column };
            }
            advance();
        } else if (m_interpolated && c == '#' && peek() == '{') {
            advance(2);
            m_state = State::DynamicStringBegin;
            return Token { Token::Type::InterpolatedStringBegin, m_file, m_token_line, m_token_column };
        } else if (c == m_start_char) {
            m_pair_depth++;
            advance();
            m_buffer->append_char(c);
        } else if (c == m_stop_char) {
            advance();
            if (m_pair_depth > 0) {
                m_pair_depth--;
                m_buffer->append_char(c);
            } else if (m_buffer->is_empty()) {
                m_state = State::Done;
                return Token { Token::Type::RBracket, m_file, m_cursor_line, m_cursor_column };
            } else {
                m_state = State::EndToken;
                return Token { Token::Type::String, m_buffer, m_file, m_token_line, m_token_column };
            }
        } else {
            m_buffer->append_char(c);
            advance();
        }
    }

    return Token { Token::Type::UnterminatedString, m_buffer, m_file, m_token_line, m_token_column };
}

Token WordArrayLexer::start_evaluation() {
    m_nested_lexer = new Lexer { *this };
    m_nested_lexer->set_stop_char('}');
    m_state = State::EvaluateEnd;
    return Token { Token::Type::EvaluateToStringBegin, m_file, m_token_line, m_token_column };
}

};
