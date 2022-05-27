#pragma once

#include "natalie_parser/creator.hpp"
#include "natalie_parser/token.hpp"

namespace NatalieParser {

class NodeWithArgs;

class BlockNode;

class Node {
public:
    enum class Type {
        Invalid,
        Alias,
        Arg,
        Array,
        ArrayPattern,
        Assignment,
        BackRef,
        Begin,
        BeginBlock,
        BeginRescue,
        Bignum,
        Block,
        BlockPass,
        Break,
        Call,
        Case,
        CaseIn,
        CaseWhen,
        Class,
        Colon2,
        Colon3,
        Constant,
        Def,
        Defined,
        EndBlock,
        EvaluateToString,
        False,
        Fixnum,
        Float,
        Hash,
        HashPattern,
        Identifier,
        If,
        InfixOp,
        Iter,
        InterpolatedRegexp,
        InterpolatedShell,
        InterpolatedString,
        InterpolatedSymbol,
        InterpolatedSymbolKey,
        KeywordArg,
        KeywordSplat,
        LogicalAnd,
        LogicalOr,
        Match,
        Module,
        MultipleAssignment,
        MultipleAssignmentArg,
        Next,
        Nil,
        NilSexp,
        Not,
        NotMatch,
        NthRef,
        OpAssign,
        OpAssignAccessor,
        OpAssignAnd,
        OpAssignOr,
        Pin,
        Range,
        Redo,
        Regexp,
        Retry,
        Return,
        SafeCall,
        Sclass,
        Self,
        ShadowArg,
        Shell,
        Splat,
        SplatValue,
        StabbyProc,
        String,
        Super,
        Symbol,
        SymbolKey,
        ToArray,
        True,
        UnaryOp,
        Undef,
        Until,
        While,
        Yield,
    };

    Node() { }

    Node(const Token &token)
        : m_token { token } { }

    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    virtual ~Node() { }

    virtual Type type() const { return Type::Invalid; }

    virtual bool is_callable() const { return false; }
    virtual bool is_assignable() const { return false; }
    virtual bool is_numeric() const { return false; }
    virtual bool is_symbol_key() const { return false; }
    virtual bool can_accept_a_block() const { return false; }
    virtual bool can_be_concatenated_to_a_string() const { return false; }
    virtual bool has_block_pass() const { return false; }

    BlockNode &as_block_node();

    virtual void transform(Creator *creator) const {
        creator->set_type("NOT_YET_IMPLEMENTED");
        creator->append_integer((int)type());
    }

    SharedPtr<String> file() const { return m_token.file(); }

    size_t line() const { return m_token.line(); }
    void set_line(size_t line) { m_token.set_line(line); }

    size_t column() const { return m_token.column(); }
    void set_column(size_t column) { m_token.set_column(column); }

    Optional<SharedPtr<String>> doc() const { return m_token.doc(); }

    const Token &token() const { return m_token; }

    const static Node &invalid() {
        if (!s_invalid)
            s_invalid = new Node;
        return *s_invalid;
    }

    operator bool() const {
        return type() != Type::Invalid;
    }

protected:
    static inline SharedPtr<Node> s_invalid {};
    Token m_token {};
};

}
