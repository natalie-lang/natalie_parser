#pragma once

#include "natalie_parser/node/begin_rescue_node.hpp"
#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class BeginNode : public Node {
public:
    BeginNode(const Token &token, BlockNode *body)
        : Node { token }
        , m_body { body } {
        assert(m_body);
    }

    BeginNode(const BeginNode &other)
        : Node { other.token() }
        , m_body { new BlockNode { other.body() } }
        , m_else_body { other.has_else_body() ? new BlockNode(other.else_body()) : nullptr }
        , m_ensure_body { other.has_ensure_body() ? new BlockNode(other.ensure_body()) : nullptr } {
        for (auto node : other.rescue_nodes())
            add_rescue_node(new BeginRescueNode { *node });
    }

    virtual Node *clone() const override {
        return new BeginNode(*this);
    }

    ~BeginNode();

    virtual Type type() const override { return Type::Begin; }

    void add_rescue_node(BeginRescueNode *node) { m_rescue_nodes.push(node); }
    bool has_rescue_nodes() const { return !m_rescue_nodes.is_empty(); }

    bool has_else_body() const { return m_else_body ? true : false; }
    bool has_ensure_body() const { return m_ensure_body ? true : false; }

    void set_else_body(BlockNode *else_body) { m_else_body = else_body; }
    void set_ensure_body(BlockNode *ensure_body) { m_ensure_body = ensure_body; }

    const BlockNode &body() const { return m_body.ref(); }
    const BlockNode &else_body() const { return m_else_body.ref(); }
    const BlockNode &ensure_body() const { return m_ensure_body.ref(); }

    const Vector<BeginRescueNode *> &rescue_nodes() const { return m_rescue_nodes; }

    virtual void transform(Creator *creator) const override;

protected:
    OwnedPtr<BlockNode> m_body {};
    OwnedPtr<BlockNode> m_else_body {};
    OwnedPtr<BlockNode> m_ensure_body {};
    Vector<BeginRescueNode *> m_rescue_nodes {};
};
}
