#pragma once

#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class CaseWhenNode : public Node {
public:
    CaseWhenNode(const Token &token, SharedPtr<Node> condition, SharedPtr<BlockNode> body)
        : Node { token }
        , m_condition { condition }
        , m_body { body } {
        assert(m_condition);
        assert(m_body);
    }

    virtual Type type() const override { return Type::CaseWhen; }

    const SharedPtr<Node> condition() const { return m_condition; }
    const SharedPtr<BlockNode> body() const { return m_body; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("when");
        creator->append(m_condition.ref());
        for (auto node : m_body->nodes())
            creator->append(node);
    }

protected:
    SharedPtr<Node> m_condition {};
    SharedPtr<BlockNode> m_body {};
};
}
