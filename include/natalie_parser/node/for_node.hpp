#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "natalie_parser/node/block_node.hpp"

namespace NatalieParser {

using namespace TM;

class ForNode : public Node {
public:
    ForNode(const Token &token, SharedPtr<Node> expr, SharedPtr<Node> vars, SharedPtr<BlockNode> body)
        : Node { token }
        , m_expr { expr }
        , m_vars { vars }
        , m_body { body } {
        assert(m_expr);
        assert(m_vars);
    }

    virtual Type type() const override { return Type::For; }

    const SharedPtr<Node> expr() const { return m_expr; }
    const SharedPtr<Node> vars() const { return m_vars; }
    const SharedPtr<BlockNode> body() const { return m_body; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("for");
        creator->append(m_expr);
        switch (m_vars->type()) {
            case Node::Type::Identifier:
                creator->with_assignment(true, [&]() { creator->append(*m_vars); });
                break;
            case Node::Type::MultipleAssignment:
                creator->append(m_vars);
                break;
            default:
                TM_UNREACHABLE();
        }
        if (!m_body->is_empty())
            creator->append(m_body->without_unnecessary_nesting());
    }

protected:
    SharedPtr<Node> m_expr {};
    SharedPtr<Node> m_vars {};
    SharedPtr<BlockNode> m_body {};
};
}
