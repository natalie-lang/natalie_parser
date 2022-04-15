#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class IdentifierNode : public Node {
public:
    IdentifierNode(const Token &token, bool is_lvar)
        : Node { token }
        , m_is_lvar { is_lvar } { }

    IdentifierNode(const IdentifierNode &other)
        : Node { other.token() }
        , m_is_lvar { other.is_lvar() } { }

    virtual Node *clone() const override {
        return new IdentifierNode(*this);
    }

    virtual Type type() const override { return Type::Identifier; }

    virtual bool can_accept_a_block() const override { return true; }

    Token::Type token_type() const { return m_token.type(); }

    SharedPtr<String> name() const { return m_token.literal_string(); }

    void prepend_to_name(char c) {
        auto literal = m_token.literal_string();
        literal->prepend_char(c);
        // FIXME: set_literal() unneeded?
        m_token.set_literal(literal);
    }

    void append_to_name(char c) {
        auto literal = m_token.literal_string();
        literal->append_char(c);
        // FIXME: set_literal() unneeded?
        m_token.set_literal(literal);
    }

    virtual bool is_callable() const override {
        switch (token_type()) {
        case Token::Type::BareName:
        case Token::Type::Constant:
            return !m_is_lvar;
        default:
            return false;
        }
    }

    bool is_lvar() const { return m_is_lvar; }
    void set_is_lvar(bool is_lvar) { m_is_lvar = is_lvar; }

    void add_to_locals(TM::Hashmap<TM::String> &locals) const {
        if (token_type() == Token::Type::BareName)
            locals.set(name()->c_str());
    }

    virtual void transform(Creator *creator) const override {
        if (creator->assignment())
            return transform_assignment(creator);
        switch (token_type()) {
        case Token::Type::BareName:
            if (is_lvar()) {
                creator->set_type("lvar");
                creator->append_symbol(name());
            } else {
                creator->set_type("call");
                creator->append_nil();
                creator->append_symbol(name());
            }
            break;
        case Token::Type::ClassVariable:
            creator->set_type("cvar");
            creator->append_symbol(name());
            break;
        case Token::Type::Constant:
            creator->set_type("const");
            creator->append_symbol(name());
            break;
        case Token::Type::GlobalVariable: {
            creator->set_type("gvar");
            creator->append_symbol(name());
            break;
        }
        case Token::Type::InstanceVariable:
            creator->set_type("ivar");
            creator->append_symbol(name());
            break;
        default:
            TM_UNREACHABLE();
        }
    }

    void transform_assignment(Creator *creator) const {
        switch (token().type()) {
        case Token::Type::BareName:
            creator->set_type("lasgn");
            break;
        case Token::Type::ClassVariable:
            creator->set_type("cvdecl");
            break;
        case Token::Type::Constant:
        case Token::Type::ConstantResolution:
            creator->set_type("cdecl");
            break;
        case Token::Type::GlobalVariable:
            creator->set_type("gasgn");
            break;
        case Token::Type::InstanceVariable:
            creator->set_type("iasgn");
            break;
        default:
            printf("got token type %d\n", (int)token().type());
            TM_UNREACHABLE();
        }
        creator->append_symbol(name());
    }

protected:
    bool m_is_lvar { false };
};
}
