#pragma once

#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class ClassNode : public Node {
public:
    ClassNode(const Token &token, Node *name, Node *superclass, BlockNode *body)
        : Node { token }
        , m_name { name }
        , m_superclass { superclass }
        , m_body { body } {
        assert(m_name);
        assert(m_superclass);
        assert(m_body);
    }

    virtual Type type() const override { return Type::Class; }

    const Node &name() const { return m_name.ref(); }
    const Node &superclass() const { return m_superclass.ref(); }
    const BlockNode &body() const { return m_body.ref(); }

    virtual void transform(Creator *creator) const override;

protected:
    OwnedPtr<Node> m_name {};
    OwnedPtr<Node> m_superclass {};
    OwnedPtr<BlockNode> m_body {};
};
}
