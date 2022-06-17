#pragma once

#include "natalie_parser/node/node.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class InfixOpNode : public Node {
public:
    InfixOpNode(const Token &token, SharedPtr<Node> left, SharedPtr<String> op, SharedPtr<Node> right)
        : Node { token }
        , m_left { left }
        , m_op { op }
        , m_right { right } {
        assert(m_left);
        assert(m_op);
        assert(m_right);
    }

    virtual Type type() const override { return Type::InfixOp; }

    virtual bool is_callable() const override { return false; }
    virtual bool can_accept_a_block() const override { return false; }

    const SharedPtr<Node> left() const { return m_left; }
    const SharedPtr<String> op() const { return m_op; }
    const SharedPtr<Node> right() const { return m_right; }

    void set_right(SharedPtr<Node> right) { m_right = right; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("call");
        creator->append(m_left.ref());
        creator->append_symbol(m_op);
        creator->append(m_right.ref());
    }

protected:
    SharedPtr<Node> m_left {};
    SharedPtr<String> m_op {};
    SharedPtr<Node> m_right {};
};
}
