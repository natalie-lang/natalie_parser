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
    ClassNode(const Token &token, SharedPtr<Node> name, SharedPtr<Node> superclass, SharedPtr<BlockNode> body)
        : Node { token }
        , m_name { name }
        , m_superclass { superclass }
        , m_body { body } {
        assert(m_name);
        assert(m_superclass);
        assert(m_body);
    }

    virtual Type type() const override { return Type::Class; }

    const SharedPtr<Node> name() const { return m_name; }
    const SharedPtr<Node> superclass() const { return m_superclass; }
    const SharedPtr<BlockNode> body() const { return m_body; }

    virtual void transform(Creator *creator) const override;

protected:
    SharedPtr<Node> m_name {};
    SharedPtr<Node> m_superclass {};
    SharedPtr<BlockNode> m_body {};
};
}
