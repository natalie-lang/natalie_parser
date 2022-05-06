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
    SclassNode(const Token &token, SharedPtr<Node> klass, SharedPtr<BlockNode> body)
        : Node { token }
        , m_klass { klass }
        , m_body { body } { }

    virtual Type type() const override { return Type::Sclass; }

    const SharedPtr<Node> klass() const { return m_klass; }
    const SharedPtr<BlockNode> body() const { return m_body; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("sclass");
        creator->append(m_klass.ref());
        for (auto node : m_body->nodes())
            creator->append(node);
    }

protected:
    SharedPtr<Node> m_klass {};
    SharedPtr<BlockNode> m_body {};
};
}
