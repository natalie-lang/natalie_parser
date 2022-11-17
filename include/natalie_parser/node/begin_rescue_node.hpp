#pragma once

#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class BeginRescueNode : public Node {
public:
    BeginRescueNode(const Token &token)
        : Node { token } { }

    virtual Type type() const override { return Type::BeginRescue; }

    void add_exception_node(SharedPtr<Node> node) {
        m_exceptions.push(node);
    }

    void set_exception_name(SharedPtr<Node> name) {
        m_name = name;
    }

    void set_body(SharedPtr<BlockNode> body) { m_body = body; }

    SharedPtr<Node> name_to_assignment() const;

    bool has_name() const { return m_name; }

    const SharedPtr<Node> name() const { return m_name; }
    const Vector<SharedPtr<Node>> &exceptions() const { return m_exceptions; }
    const SharedPtr<BlockNode> body() const { return m_body; }

    virtual void transform(Creator *creator) const override;

protected:
    SharedPtr<Node> m_name {};
    Vector<SharedPtr<Node>> m_exceptions {};
    SharedPtr<BlockNode> m_body {};
};
}
