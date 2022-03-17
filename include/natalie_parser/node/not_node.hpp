#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class NotNode : public Node {
public:
    NotNode(const Token &token, Node *expression)
        : Node { token }
        , m_expression { expression } {
        assert(m_expression);
    }

    virtual Type type() const override { return Type::Not; }

    const Node &expression() const { return m_expression.ref(); }

    virtual void transform(Creator *creator) const override {
        creator->set_type("not");
        creator->append(m_expression.ref());
    }

protected:
    OwnedPtr<Node> m_expression {};
};
}