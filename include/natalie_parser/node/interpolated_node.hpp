#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class InterpolatedNode : public Node {
public:
    InterpolatedNode(const Token &token)
        : Node { token } { }

    InterpolatedNode(const InterpolatedNode &other)
        : InterpolatedNode { other.token() } {
        for (auto node : other.nodes())
            add_node(node->clone());
    }

    virtual Node *clone() const override {
        return new InterpolatedNode(*this);
    }

    ~InterpolatedNode() {
        for (auto node : m_nodes)
            delete node;
    }

    bool is_empty() const { return m_nodes.is_empty(); }

    void add_node(Node *node) { m_nodes.push(node); };

    const Vector<Node *> &nodes() const { return m_nodes; }

protected:
    Vector<Node *> m_nodes {};
};
}
