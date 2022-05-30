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
    BeginNode(const Token &token, SharedPtr<BlockNode> body)
        : Node { token }
        , m_body { body } {
        assert(m_body);
    }

    virtual Type type() const override { return Type::Begin; }

    bool can_be_simple_block() const {
        return !has_rescue_nodes() && !has_else_body() && !has_ensure_body();
    }

    void add_rescue_node(SharedPtr<BeginRescueNode> node) { m_rescue_nodes.push(node); }
    bool has_rescue_nodes() const { return !m_rescue_nodes.is_empty(); }

    bool has_else_body() const { return m_else_body ? true : false; }
    bool has_ensure_body() const { return m_ensure_body ? true : false; }

    void set_else_body(SharedPtr<BlockNode> else_body) { m_else_body = else_body; }
    void set_ensure_body(SharedPtr<BlockNode> ensure_body) { m_ensure_body = ensure_body; }

    const SharedPtr<BlockNode> body() const { return m_body; }
    const SharedPtr<BlockNode> else_body() const { return m_else_body; }
    const SharedPtr<BlockNode> ensure_body() const { return m_ensure_body; }

    const Vector<SharedPtr<BeginRescueNode>> &rescue_nodes() const { return m_rescue_nodes; }

    virtual void transform(Creator *creator) const override;

protected:
    SharedPtr<BlockNode> m_body {};
    SharedPtr<BlockNode> m_else_body {};
    SharedPtr<BlockNode> m_ensure_body {};
    Vector<SharedPtr<BeginRescueNode>> m_rescue_nodes {};
};
}
