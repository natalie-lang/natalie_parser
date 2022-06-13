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
    CaseInNode(const Token &token, SharedPtr<Node> pattern, SharedPtr<BlockNode> body)
        : Node { token }
        , m_pattern { pattern }
        , m_body { body } {
        assert(m_pattern);
        assert(m_body);
    }

    virtual Type type() const override { return Type::CaseIn; }

    const SharedPtr<Node> pattern() const { return m_pattern; }
    const SharedPtr<BlockNode> body() const { return m_body; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("in");
        creator->append(m_pattern.ref());
        if (!m_body->is_empty())
            for (auto node : m_body->nodes())
                creator->append(node);
        else
            creator->append_nil();
    }

protected:
    SharedPtr<Node> m_pattern {};
    SharedPtr<BlockNode> m_body {};
};
}
