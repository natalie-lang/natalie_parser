#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class SplatNode : public Node {
public:
    SplatNode(const Token &token)
        : Node { token } { }

    SplatNode(const Token &token, Node *node)
        : Node { token }
        , m_node { node } {
        assert(m_node);
    }

    SplatNode(const SplatNode &other)
        : SplatNode {
            other.token(),
            other.node().clone(),
        } { }

    virtual Node *clone() const override {
        return new SplatNode(*this);
    }

    virtual Type type() const override { return Type::Splat; }

    virtual bool is_assignable() const override { return true; }

    const Node &node() const {
        if (m_node)
            return m_node.ref();
        return Node::invalid();
    }

    virtual void transform(Creator *creator) const override {
        creator->set_type("splat");
        if (m_node)
            creator->append(m_node.ref());
    }

protected:
    OwnedPtr<Node> m_node {};
};
}
