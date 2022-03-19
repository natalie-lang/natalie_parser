#pragma once

#include "natalie_parser/token.hpp"
#include "tm/shared_ptr.hpp"
#include "tm/vector.hpp"

namespace NatalieParser {

class InterpolatedStringLexer {
public:
    InterpolatedStringLexer(Token &token)
        : m_input { token.literal_string() }
        , m_file { token.file() }
        , m_line { token.line() }
        , m_column { token.column() }
        , m_size { strlen(token.literal()) } { }

    SharedPtr<Vector<Token>> tokens();

private:
    void tokenize_interpolation(SharedPtr<Vector<Token>>);

    char current_char() {
        if (m_index >= m_size)
            return 0;
        return m_input->at(m_index);
    }

    char peek() {
        if (m_index + 1 >= m_size)
            return 0;
        return m_input->at(m_index + 1);
    }

    SharedPtr<String> m_input;
    SharedPtr<String> m_file;
    size_t m_line { 0 };
    size_t m_column { 0 };
    size_t m_size { 0 };
    size_t m_index { 0 };
};

}
