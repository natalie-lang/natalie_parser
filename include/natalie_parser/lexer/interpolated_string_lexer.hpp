#pragma once

#include "natalie_parser/lexer.hpp"
#include "natalie_parser/token.hpp"
#include "tm/shared_ptr.hpp"
#include "tm/vector.hpp"

namespace NatalieParser {

class InterpolatedStringLexer : public Lexer {
public:
    InterpolatedStringLexer(Lexer &parent_lexer, char start_char, char stop_char, Token::Type end_type)
        : Lexer { parent_lexer }
        , m_end_type { end_type } {
        set_nested_lexer(nullptr);
        set_start_char(start_char);
        set_stop_char(stop_char);
    }

    InterpolatedStringLexer(Lexer &parent_lexer, Token string_token, Token::Type end_type)
        : Lexer { string_token.literal_string(), parent_lexer.file() }
        , m_end_type { end_type } {
        set_nested_lexer(nullptr);
        set_stop_char(0);
    }

private:
    virtual Token build_next_token() override;
    Token consume_string();
    Token start_evaluation();

    virtual bool skip_whitespace() override { return false; }

    enum class State {
        InProgress,
        EvaluateBegin,
        EvaluateEnd,
        EndToken,
        Done,
    };

    State m_state { State::InProgress };
    Token::Type m_end_type;
};
}
