#include <limits>

#include "natalie_parser/lexer.hpp"
#include "natalie_parser/token.hpp"

namespace NatalieParser {

void InterpolatedStringLexer::tokens(Vector<Token> &tokens) {
    SharedPtr<String> raw = new String("");
    while (m_index < m_size) {
        char c = current_char();
        if (c == '#' && peek() == '{') {
            if (!raw->is_empty() || tokens.is_empty()) {
                tokens.push(Token { Token::Type::String, new String(raw.ref()), m_file, m_line, m_column });
                *raw = "";
            }
            m_index += 2;
            tokenize_interpolation(tokens);
        } else {
            raw->append_char(c);
            m_index++;
        }
    }
    if (!raw->is_empty())
        tokens.push(Token { Token::Type::String, raw, m_file, m_line, m_column });
}

void InterpolatedStringLexer::tokenize_interpolation(Vector<Token> &tokens) {
    size_t start_index = m_index;
    size_t curly_brace_count = 1;
    while (m_index < m_size && curly_brace_count > 0) {
        char c = current_char();
        switch (c) {
        case '{':
            curly_brace_count++;
            break;
        case '}':
            curly_brace_count--;
            break;
        case '\\':
            m_index++;
            break;
        }
        m_index++;
    }
    if (curly_brace_count > 0) {
        fprintf(stderr, "missing } in string interpolation in %s#%zu\n", m_file->c_str(), m_line + 1);
        abort();
    }
    // m_input = "#{:foo} bar"
    //                   ^ m_index = 7
    //              ^ start_index = 2
    // part = ":foo" (len = 4)
    size_t len = m_index - start_index - 1;
    auto part = m_input->substring(start_index, len);
    auto lexer = Lexer { new String(part), m_file };
    tokens.push(Token { Token::Type::EvaluateToStringBegin, m_file, m_line, m_column });
    auto part_tokens = lexer.tokens();
    for (auto token : *part_tokens) {
        if (token.is_eof()) {
            tokens.push(Token { Token::Type::Eol, m_file, m_line, m_column });
            break;
        } else {
            tokens.push(token);
        }
    }
    tokens.push(Token { Token::Type::EvaluateToStringEnd, m_file, m_line, m_column });
}

}
