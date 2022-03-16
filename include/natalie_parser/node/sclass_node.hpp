#pragma once

#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class SclassNode : public Node {
public:
    SclassNode(const Token &token, Node *klass, BlockNode *body)
        : Node { token }
        , m_klass { klass }
        , m_body { body } { }

    virtual Type type() const override { return Type::Sclass; }

    const Node &klass() const { return m_klass.ref(); }
    const BlockNode &body() const { return m_body.ref(); }

    virtual void transform(Creator *creator) const override {
        creator->set_type("sclass");
        creator->append(m_klass.ref());
        for (auto node : m_body->nodes())
            creator->append(node);
    }

protected:
    OwnedPtr<Node> m_klass {};
    OwnedPtr<BlockNode> m_body {};
};
}
