#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class BlockNode : public Node {
public:
    BlockNode(const Token &token)
        : Node { token } { }

    BlockNode(const Token &token, Node *single_node)
        : Node { token } {
        add_node(single_node);
    }

    ~BlockNode() {
        for (auto node : m_nodes)
            delete node;
    }

    void add_node(Node *node) {
        m_nodes.push(node);
    }

    virtual Type type() const override { return Type::Block; }

    Vector<Node *> &nodes() { return m_nodes; }
    bool is_empty() const { return m_nodes.is_empty(); }

    bool has_one_node() const { return m_nodes.size() == 1; }

    Node *without_unnecessary_nesting() {
        if (has_one_node())
            return m_nodes[0];
        else
            return this;
    }

    virtual void transform(Creator *creator) const override {
        creator->set_type("block");
        for (auto node : m_nodes)
            creator->append(node);
    }

protected:
    Vector<Node *> m_nodes {};
};
}
