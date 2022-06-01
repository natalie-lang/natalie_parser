#pragma once

#include "natalie_parser/lexer.hpp"
#include "natalie_parser/token.hpp"
#include "tm/shared_ptr.hpp"
#include "tm/vector.hpp"

namespace NatalieParser {

class WordArrayLexer : public Lexer {
public:
    WordArrayLexer(Lexer &parent_lexer, char start_char, char stop_char, bool interpolated)
        : Lexer { parent_lexer }
        , m_interpolated { interpolated }
        , m_start_char { start_char } {
        set_nested_lexer(nullptr);
        set_stop_char(stop_char);
    }

private:
    virtual Token build_next_token() override;
    Token consume_array();

    virtual bool skip_whitespace() override { return false; }

    bool interpolated() const { return m_interpolated; }

    // states
    enum class State {
        InProgress,
        DynamicStringBegin,
        DynamicStringInProgress,
        DynamicStringEnd,
        EvaluateBegin,
        EvaluateEnd,
        EndToken,
        Done,
    };

    // transitions
    Token in_progress_start_dynamic_string();
    Token start_evaluation();
    Token dynamic_string_finish();
    Token in_progress_finish();

    State m_state { State::InProgress };

    // if this is true, then process #{...} interpolation
    bool m_interpolated { false };

    // if we encounter the m_start_char within the array,
    // then increment m_pair_depth
    char m_start_char { 0 };
    int m_pair_depth { 0 };
    SharedPtr<String> m_buffer;
};
}
