#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class BlockPassNode : public Node {
public:
    BlockPassNode(const Token &token, Node *node)
        : Node { token }
        , m_node { node } {
        assert(m_node);
    }

    BlockPassNode(const BlockPassNode &other)
        : BlockPassNode {
            other.token(),
            other.node().clone(),
        } { }

    virtual Node *clone() const override {
        return new BlockPassNode(*this);
    }

    virtual Type type() const override { return Type::BlockPass; }

    const Node &node() const { return m_node.ref(); }

    virtual void transform(Creator *creator) const override {
        creator->set_type("block_pass");
        creator->append(m_node.ref());
    }

protected:
    OwnedPtr<Node> m_node {};
};
}
