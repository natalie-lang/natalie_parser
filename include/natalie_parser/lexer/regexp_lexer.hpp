#pragma once

#include "natalie_parser/lexer.hpp"
#include "natalie_parser/token.hpp"
#include "tm/shared_ptr.hpp"
#include "tm/vector.hpp"

namespace NatalieParser {

class RegexpLexer : public Lexer {
public:
    RegexpLexer(Lexer &parent_lexer, char stop_char)
        : Lexer { parent_lexer } {
        set_nested_lexer(nullptr);
        set_stop_char(stop_char);
    }

private:
    virtual Token build_next_token() override;
    Token consume_regexp();
    String *consume_options();

    virtual bool skip_whitespace() override { return false; }

    enum class State {
        InProgress,
        EvaluateBegin,
        EvaluateEnd,
        EndToken,
        Done,
    };

    State m_state { State::InProgress };
    SharedPtr<String> m_options {};
};
}
