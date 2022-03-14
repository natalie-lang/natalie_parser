#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
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

    ~LogicalOrNode() {
        delete m_left;
        delete m_right;
    }

    virtual Type type() const override { return Type::LogicalOr; }

    Node *left() const { return m_left; }
    Node *right() const { return m_right; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("or");
        creator->append(m_left);
        creator->append(m_right);
    }

protected:
    Node *m_left { nullptr };
    Node *m_right { nullptr };
};
}
