#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class HashNode : public Node {
public:
    HashNode(const Token &token)
        : Node { token } { }

    ~HashNode() {
        for (auto node : m_nodes)
            delete node;
    }

    virtual Type type() const override { return Type::Hash; }

    void add_node(Node *node) {
        m_nodes.push(node);
    }

    Vector<Node *> &nodes() { return m_nodes; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("hash");
        for (auto node : m_nodes)
            creator->append(node);
    }

protected:
    Vector<Node *> m_nodes {};
};
}
