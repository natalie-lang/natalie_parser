#pragma once

#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/identifier_node.hpp"
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

    BeginRescueNode(const BeginRescueNode &other)
        : Node { other.token() }
        , m_name { other.has_name() ? new IdentifierNode(other.name()) : nullptr }
        , m_body { new BlockNode { other.body() } } {
        for (auto node : other.exceptions())
            add_exception_node(node->clone());
    }

    virtual Node *clone() const override {
        return new BeginRescueNode(*this);
    }

    ~BeginRescueNode();

    virtual Type type() const override { return Type::BeginRescue; }

    void add_exception_node(Node *node) {
        m_exceptions.push(node);
    }

    void set_exception_name(IdentifierNode *name) {
        m_name = name;
    }

    void set_body(BlockNode *body) { m_body = body; }

    Node *name_to_node() const;

    bool has_name() const { return m_name; }

    const IdentifierNode &name() const {
        assert(m_name);
        return m_name.ref();
    }

    const Vector<Node *> &exceptions() const { return m_exceptions; }

    const BlockNode &body() const {
        assert(m_body);
        return m_body.ref();
    }

    virtual void transform(Creator *creator) const override;

protected:
    OwnedPtr<IdentifierNode> m_name {};
    Vector<Node *> m_exceptions {};
    OwnedPtr<BlockNode> m_body {};
};
}
