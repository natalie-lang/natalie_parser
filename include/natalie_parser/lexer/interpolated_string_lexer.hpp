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
        set_start_char(start_char == stop_char ? 0 : start_char);
        set_stop_char(stop_char);
    }

    // used for lexing a Heredoc
    InterpolatedStringLexer(Lexer &parent_lexer, Token string_token, Token::Type end_type)
        : Lexer { string_token.literal_string(), parent_lexer.file() }
        , m_end_type { end_type }
        , m_alters_parent_cursor_position { false } {
        set_cursor_line(parent_lexer.cursor_line() + 1); // the line after the heredoc delimiter
        set_nested_lexer(nullptr);
        set_stop_char(0);
    }

    virtual bool alters_parent_cursor_position() override { return m_alters_parent_cursor_position; }

private:
    virtual Token build_next_token() override;
    Token consume_string();
    Token start_evaluation();
    Token stop_evaluation();
    Token finish();

    virtual bool skip_whitespace() override { return false; }

    /**
     * a little state machine
     * stateDiagram-v2
     *     [*] --> InProgress
     *     InProgress --> EvaluateBegin
     *     InProgress --> EndToken
     *     EvaluateBegin --> EvaluateEnd
     *     EvaluateEnd --> InProgress
     *     EndToken --> Done
     *     Done --> [*]
     */
    enum class State {
        InProgress,
        EvaluateBegin,
        EvaluateEnd,
        EndToken,
        Done,
    };

    State m_state { State::InProgress };
    Token::Type m_end_type;
    bool m_alters_parent_cursor_position { true };
};
}
