#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class LogicalOrNode : public Node {
public:
    LogicalOrNode(const Token &token, Node *left, Node *right)
        : Node { token }
        , m_left { left }
        , m_right { right } {
        assert(m_left);
        assert(m_right);
    }

    virtual Type type() const override { return Type::LogicalOr; }

    const Node &left() const { return m_left.ref(); }
    const Node &right() const { return m_right.ref(); }

    void set_right(Node *right) { m_right = right; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("or");
        creator->append(m_left.ref());
        creator->append(m_right.ref());
    }

protected:
    OwnedPtr<Node> m_left {};
    OwnedPtr<Node> m_right {};
};
}
