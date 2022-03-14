#pragma once

#include "natalie_parser/node/begin_rescue_node.hpp"
#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
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

    ~BeginNode();

    virtual Type type() const override { return Type::Begin; }

    void add_rescue_node(BeginRescueNode *node) { m_rescue_nodes.push(node); }
    bool no_rescue_nodes() const { return m_rescue_nodes.size() == 0; }

    bool has_ensure_body() const { return m_ensure_body ? true : false; }
    void set_else_body(BlockNode *else_body) { m_else_body = else_body; }
    void set_ensure_body(BlockNode *ensure_body) { m_ensure_body = ensure_body; }

    BlockNode *body() const { return m_body; }
    BlockNode *else_body() const { return m_else_body; }
    BlockNode *ensure_body() const { return m_ensure_body; }

    Vector<BeginRescueNode *> &rescue_nodes() { return m_rescue_nodes; }

    virtual void transform(Creator *creator) const override;

protected:
    BlockNode *m_body { nullptr };
    BlockNode *m_else_body { nullptr };
    BlockNode *m_ensure_body { nullptr };
    Vector<BeginRescueNode *> m_rescue_nodes {};
};
}
