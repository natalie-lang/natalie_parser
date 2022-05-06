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

    BlockNode(const Token &token, SharedPtr<Node> single_node)
        : Node { token } {
        add_node(single_node);
    }

    virtual Type type() const override { return Type::Block; }

    const Vector<SharedPtr<Node>> &nodes() const { return m_nodes; }

    void add_node(SharedPtr<Node> node) {
        m_nodes.push(node);
    }

    SharedPtr<Node> take_first_node() {
        return m_nodes.pop_front();
    }

    bool is_empty() const { return m_nodes.is_empty(); }

    bool has_one_node() const { return m_nodes.size() == 1; }
    SharedPtr<Node> first() const { return m_nodes.at(0); }

    const Node &without_unnecessary_nesting() {
        if (has_one_node())
            return *first();
        else
            return *this;
    }

    virtual void transform(Creator *creator) const override {
        creator->set_type("block");
        for (auto node : m_nodes)
            creator->append(node);
    }

protected:
    Vector<SharedPtr<Node>> m_nodes {};
};
}
