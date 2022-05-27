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
        : Node { other.token() } {
        for (auto node : other.nodes())
            add_node(node);
    }

    bool is_empty() const { return m_nodes.is_empty(); }

    void prepend_node(SharedPtr<Node> node) { m_nodes.push_front(node); };
    void add_node(SharedPtr<Node> node) { m_nodes.push(node); };

    const Vector<SharedPtr<Node>> &nodes() const { return m_nodes; }

protected:
    Vector<SharedPtr<Node>> m_nodes {};
};
}
