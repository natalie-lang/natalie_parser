#pragma once

#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class WhileNode : public Node {
public:
    WhileNode(const Token &token, SharedPtr<Node> condition, SharedPtr<BlockNode> body, bool pre)
        : Node { token }
        , m_condition { condition }
        , m_body { body }
        , m_pre { pre } {
        assert(m_condition);
        assert(m_body);
    }

    virtual Type type() const override { return Type::While; }

    const SharedPtr<Node> condition() const { return m_condition; }
    const SharedPtr<BlockNode> body() const { return m_body; }
    bool pre() const { return m_pre; }

    virtual void transform(Creator *creator) const override {
        if (type() == Node::Type::Until)
            creator->set_type("until");
        else
            creator->set_type("while");
        creator->append(m_condition.ref());
        if (m_body->is_empty())
            creator->append_nil();
        else
            creator->append(m_body->without_unnecessary_nesting());
        if (m_pre)
            creator->append_true();
        else
            creator->append_false();
    }

protected:
    SharedPtr<Node> m_condition {};
    SharedPtr<BlockNode> m_body {};
    bool m_pre { false };
};
}
