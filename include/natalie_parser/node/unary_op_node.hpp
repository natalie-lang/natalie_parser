#pragma once

#include "natalie_parser/node/node.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class UnaryOpNode : public Node {
public:
    UnaryOpNode(const Token &token, SharedPtr<String> op, Node *right)
        : Node { token }
        , m_op { op }
        , m_right { right } {
        assert(m_op);
        assert(m_right);
    }

    virtual Type type() const override { return Type::UnaryOp; }

    const SharedPtr<String> op() const { return m_op; }
    const Node &right() const { return m_right.ref(); }

    void set_right(Node *right) { m_right = right; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("call");
        creator->append(m_right.ref());
        creator->append_symbol(m_op);
    }

protected:
    SharedPtr<String> m_op {};
    OwnedPtr<Node> m_right {};
};
}
