#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class ArrayNode : public Node {
public:
    ArrayNode(const Token &token)
        : Node { token } { }

    virtual Type type() const override { return Type::Array; }

    void add_node(SharedPtr<Node> node) {
        m_nodes.push(node);
    }

    const Vector<SharedPtr<Node>> &nodes() const { return m_nodes; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("array");
        for (auto node : m_nodes)
            creator->append(node);
    }

protected:
    Vector<SharedPtr<Node>> m_nodes {};
};
}
