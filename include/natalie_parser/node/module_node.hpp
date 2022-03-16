#pragma once

#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class ModuleNode : public Node {
public:
    ModuleNode(const Token &token, Node *name, BlockNode *body)
        : Node { token }
        , m_name { name }
        , m_body { body } { }

    virtual Type type() const override { return Type::Module; }

    const Node &name() const { return m_name.ref(); }
    const BlockNode &body() const { return m_body.ref(); }

    virtual void transform(Creator *creator) const override;

protected:
    OwnedPtr<Node> m_name {};
    OwnedPtr<BlockNode> m_body {};
};
}
