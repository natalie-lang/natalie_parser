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
        advance(); // }
        if (current_char() == m_stop_char) {
            advance();
            m_state = State::EndToken;
        } else {
            m_state = State::InProgress;
        }
        return Token { Token::Type::EvaluateToStringEnd, m_file, m_token_line, m_token_column };
    case State::EndToken:
        m_state = State::Done;
        return Token { m_end_type, m_file, m_cursor_line, m_cursor_column };
    case State::Done:
        return Token { Token::Type::Eof, m_file, m_cursor_line, m_cursor_column };
    }
    TM_UNREACHABLE();
}

Token InterpolatedStringLexer::consume_string() {
    SharedPtr<String> buf = new String;
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
    while (auto c = current_char()) {
        if (c == '\\') {
            c = next();
            if (c >= '0' && c <= '7') {
                auto number = consume_octal_number(3);
                buf->append_char(number);
            } else if (c == 'x') {
                // hex: 1-2 digits
                advance();
                auto number = consume_hex_number(2);
                buf->append_char(number);
            } else if (c == 'u') {
                c = next();
                if (c == '{') {
                    c = next();
                    // unicode characters, space separated, 1-6 hex digits
                    while (c != '}') {
                        if (!isxdigit(c))
                            return Token { Token::Type::InvalidUnicodeEscape, c, m_file, m_cursor_line, m_cursor_column };
                        auto codepoint = consume_hex_number(6);
                        utf32_codepoint_to_utf8(buf.ref(), codepoint);
                        c = current_char();
                        while (c == ' ')
                            c = next();
                    }
                    if (c == '}')
                        advance();
                } else {
                    // unicode: 4 hex digits
                    auto codepoint = consume_hex_number(4);
                    utf32_codepoint_to_utf8(buf.ref(), codepoint);
                }
            } else {
                switch (c) {
                case 'a':
                    buf->append_char('\a');
                    break;
                case 'b':
                    buf->append_char('\b');
                    break;
                case 'c':
                case 'C': {
                    int num = control_character(false);
                    if (num == -1)
                        return Token { Token::Type::InvalidCharacterEscape, current_char(), m_file, m_cursor_line, m_cursor_column };
                    buf->append_char((unsigned char)num);
                    break;
                }
                case 'e':
                    buf->append_char('\e');
                    break;
                case 'f':
                    buf->append_char('\f');
                    break;
                case 'M': {
                    c = next();
                    if (c != '-')
                        return Token { Token::Type::InvalidCharacterEscape, c, m_file, m_cursor_line, m_cursor_column };
                    c = next();
                    int num = 0;
                    if (c == '\\' && (peek() == 'c' || peek() == 'C')) {
                        advance();
                        num = control_character(true);
                    } else {
                        num = (int)c + 128;
                    }
                    buf->append_char((unsigned char)num);
                    break;
                }
                case 'n':
                    buf->append_char('\n');
                    break;
                case 'r':
                    buf->append_char('\r');
                    break;
                case 's':
                    buf->append_char((unsigned char)32);
                    break;
                case 't':
                    buf->append_char('\t');
                    break;
                case 'v':
                    buf->append_char('\v');
                    break;
                case '\n':
                    break;
                default:
                    buf->append_char(c);
                    break;
                }
                advance();
            }
        } else if (c == '#' && peek() == '{') {
            if (buf->is_empty()) {
                advance(2);
                return start_evaluation();
            }
            auto token = Token { Token::Type::String, buf, m_file, m_token_line, m_token_column };
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
                m_state = State::EndToken;
                return Token { Token::Type::String, buf, m_file, m_token_line, m_token_column };
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
        return Token { Token::Type::String, buf, m_file, m_token_line, m_token_column };
    }

    return Token { Token::Type::UnterminatedString, buf, m_file, m_token_line, m_token_column };
}

Token InterpolatedStringLexer::start_evaluation() {
    m_nested_lexer = new Lexer { *this };
    m_nested_lexer->set_start_char('{');
    m_nested_lexer->set_stop_char('}');
    m_state = State::EvaluateEnd;
    return Token { Token::Type::EvaluateToStringBegin, m_file, m_token_line, m_token_column };
}

};
