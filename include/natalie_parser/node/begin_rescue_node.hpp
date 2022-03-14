#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/identifier_node.hpp"
#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class BeginRescueNode : public Node {
public:
    BeginRescueNode(const Token &token)
        : Node { token } { }

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

    IdentifierNode *name() const { return m_name; }
    Vector<Node *> &exceptions() { return m_exceptions; }
    BlockNode *body() const { return m_body; }

    virtual void transform(Creator *creator) const override;

protected:
    IdentifierNode *m_name { nullptr };
    Vector<Node *> m_exceptions {};
    BlockNode *m_body { nullptr };
};
}
