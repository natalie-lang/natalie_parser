#pragma once

#include "natalie_parser/lexer.hpp"
#include "natalie_parser/token.hpp"
#include "tm/shared_ptr.hpp"
#include "tm/vector.hpp"

namespace NatalieParser {

class WordArrayLexer : public Lexer {
public:
    WordArrayLexer(Lexer &parent_lexer, char stop_char, bool interpolated)
        : Lexer { parent_lexer }
        , m_interpolated { interpolated } {
        set_nested_lexer(nullptr);
        set_stop_char(stop_char);
    }

private:
    virtual Token build_next_token() override;
    Token consume_array();
    Token start_evaluation();

    bool interpolated() const { return m_interpolated; }

    enum class State {
        InProgress,
        DynamicStringBegin,
        DynamicStringEnd,
        EvaluateBegin,
        EvaluateEnd,
        EndToken,
        Done,
    };

    State m_state { State::InProgress };
    bool m_interpolated { false };
};
}
