#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class HashNode : public Node {
public:
    HashNode(const Token &token, bool bare)
        : Node { token }
        , m_bare { bare } { }

    virtual Type type() const override { return Type::Hash; }

    void add_node(SharedPtr<Node> node) {
        m_nodes.push(node);
    }

    const Vector<SharedPtr<Node>> &nodes() const { return m_nodes; }

    virtual void transform(Creator *creator) const override {
        if (m_bare)
            creator->set_type("bare_hash");
        else
            creator->set_type("hash");
        for (auto node : m_nodes)
            creator->append(node);
    }

protected:
    Vector<SharedPtr<Node>> m_nodes {};
    bool m_bare { false };
};
}
