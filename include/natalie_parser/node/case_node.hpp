#pragma once

#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class CaseNode : public Node {
public:
    CaseNode(const Token &token, SharedPtr<Node> subject)
        : Node { token }
        , m_subject { subject } {
        assert(m_subject);
    }

    virtual Type type() const override { return Type::Case; }

    void add_node(SharedPtr<Node> node) {
        m_nodes.push(node);
    }

    void set_else_node(SharedPtr<BlockNode> node) {
        m_else_node = node;
    }

    const SharedPtr<Node> subject() const { return m_subject; }
    Vector<SharedPtr<Node>> &nodes() { return m_nodes; }
    const SharedPtr<BlockNode> else_node() const { return m_else_node; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("case");
        creator->append(m_subject.ref());
        for (auto when_node : m_nodes)
            creator->append(when_node);
        if (m_else_node)
            creator->append(m_else_node->without_unnecessary_nesting());
        else
            creator->append_nil();
    }

protected:
    SharedPtr<Node> m_subject {};
    Vector<SharedPtr<Node>> m_nodes {};
    SharedPtr<BlockNode> m_else_node {};
};
}
