#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class CaseNode : public Node {
public:
    CaseNode(const Token &token, Node *subject)
        : Node { token }
        , m_subject { subject } {
        assert(m_subject);
    }

    ~CaseNode() {
        delete m_subject;
        for (auto node : m_nodes)
            delete node;
        delete m_else_node;
    }

    virtual Type type() const override { return Type::Case; }

    void add_node(Node *node) {
        m_nodes.push(node);
    }

    void set_else_node(BlockNode *node) {
        m_else_node = node;
    }

    Node *subject() const { return m_subject; }
    Vector<Node *> &nodes() { return m_nodes; }
    BlockNode *else_node() const { return m_else_node; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("case");
        creator->append(m_subject);
        for (auto when_node : m_nodes)
            creator->append(when_node);
        if (m_else_node)
            creator->append(m_else_node->without_unnecessary_nesting());
        else
            creator->append_nil();
    }

protected:
    Node *m_subject { nullptr };
    Vector<Node *> m_nodes {};
    BlockNode *m_else_node { nullptr };
};
}
