#pragma once

#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class CaseInNode : public Node {
public:
    CaseInNode(const Token &token, Node *pattern, BlockNode *body)
        : Node { token }
        , m_pattern { pattern }
        , m_body { body } {
        assert(m_pattern);
        assert(m_body);
    }

    virtual Type type() const override { return Type::CaseIn; }

    const Node &pattern() const { return m_pattern.ref(); }
    const BlockNode &body() const { return m_body.ref(); }

    virtual void transform(Creator *creator) const override {
        creator->set_type("in");
        creator->append(m_pattern.ref());
        for (auto node : m_body->nodes())
            creator->append(node);
    }

protected:
    OwnedPtr<Node> m_pattern {};
    OwnedPtr<BlockNode> m_body {};
};
}
