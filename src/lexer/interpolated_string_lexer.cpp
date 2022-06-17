#include "natalie_parser/lexer/interpolated_string_lexer.hpp"
#include "natalie_parser/token.hpp"

namespace NatalieParser {

Token InterpolatedStringLexer::build_next_token() {
    switch (m_state) {
    case State::InProgress:
        return consume_string();
    case State::EvaluateBegin:
        return start_evaluation();
    case State::EvaluateEnd:
        return stop_evaluation();
    case State::EndToken:
        return finish();
    case State::Done:
        return Token { Token::Type::Eof, m_file, m_cursor_line, m_cursor_column, m_whitespace_precedes };
    }
    TM_UNREACHABLE();
}

Token InterpolatedStringLexer::consume_string() {
    SharedPtr<String> buf = new String;
    while (auto c = current_char()) {
        if (c == '\\') {
            advance(); // backslash
            auto result = consume_escaped_byte(*buf);
            if (!result.first)
                return Token { result.second, current_char(), m_file, m_cursor_line, m_cursor_column, m_whitespace_precedes };
        } else if (c == '#' && peek() == '{') {
            if (buf->is_empty()) {
                advance(2);
                return start_evaluation();
            }
            auto token = Token { Token::Type::String, buf, m_file, m_token_line, m_token_column, m_whitespace_precedes };
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
            } else if (buf->is_empty()) {
                return finish();
            } else {
                m_state = State::EndToken;
                return Token { Token::Type::String, buf, m_file, m_token_line, m_token_column, m_whitespace_precedes };
            }
        } else {
            buf->append_char(c);
            advance();
        }
    }

    // Heredocs don't use a stop char --
    // they just give us the whole input and we consume everything.
    if (m_stop_char == 0) {
        advance();
        m_state = State::EndToken;
        return Token { Token::Type::String, buf, m_file, m_token_line, m_token_column, m_whitespace_precedes };
    }

    return Token { Token::Type::UnterminatedString, buf, m_file, m_token_line, m_token_column, m_whitespace_precedes };
}

Token InterpolatedStringLexer::start_evaluation() {
    m_nested_lexer = new Lexer { *this, '{', '}' };
    m_state = State::EvaluateEnd;
    return Token { Token::Type::EvaluateToStringBegin, m_file, m_token_line, m_token_column, m_whitespace_precedes };
}

Token InterpolatedStringLexer::stop_evaluation() {
    advance(); // }
    m_state = State::InProgress;
    return Token { Token::Type::EvaluateToStringEnd, m_file, m_token_line, m_token_column, m_whitespace_precedes };
}

Token InterpolatedStringLexer::finish() {
    m_state = State::Done;
    return Token { m_end_type, m_file, m_cursor_line, m_cursor_column, m_whitespace_precedes };
}

};
