#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class IfNode : public Node {
public:
    IfNode(const Token &token, Node *condition, Node *true_expr, Node *false_expr)
        : Node { token }
        , m_condition { condition }
        , m_true_expr { true_expr }
        , m_false_expr { false_expr } {
        assert(m_condition);
        assert(m_true_expr);
        assert(m_false_expr);
    }

    virtual Type type() const override { return Type::If; }

    const Node &condition() const { return m_condition.ref(); }
    const Node &true_expr() const { return m_true_expr.ref(); }
    const Node &false_expr() const { return m_false_expr.ref(); }

    virtual void transform(Creator *creator) const override {
        creator->set_type("if");
        creator->append(m_condition.ref());
        creator->append(m_true_expr.ref());
        creator->append(m_false_expr.ref());
    }

protected:
    OwnedPtr<Node> m_condition {};
    OwnedPtr<Node> m_true_expr {};
    OwnedPtr<Node> m_false_expr {};
};
}
