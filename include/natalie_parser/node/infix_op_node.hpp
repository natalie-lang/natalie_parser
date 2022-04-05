#pragma once

#include "natalie_parser/node/node.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class InfixOpNode : public Node {
public:
    InfixOpNode(const Token &token, Node *left, SharedPtr<String> op, Node *right)
        : Node { token }
        , m_left { left }
        , m_op { op }
        , m_right { right } {
        assert(m_left);
        assert(m_op);
        assert(m_right);
    }

    virtual Type type() const override { return Type::InfixOp; }

    const Node &left() const { return m_left.ref(); }
    const SharedPtr<String> op() const { return m_op; }
    const Node &right() const { return m_right.ref(); }

    void set_right(Node *right) { m_right = right; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("call");
        creator->append(m_left.ref());
        creator->append_symbol(m_op);
        creator->append(m_right.ref());
    }

protected:
    OwnedPtr<Node> m_left {};
    SharedPtr<String> m_op {};
    OwnedPtr<Node> m_right {};
};
}
