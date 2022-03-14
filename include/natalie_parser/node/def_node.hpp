#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class DefNode : public NodeWithArgs {
public:
    DefNode(const Token &token, Node *self_node, SharedPtr<String> name, Vector<Node *> &args, BlockNode *body)
        : NodeWithArgs { token, args }
        , m_self_node { self_node }
        , m_name { name }
        , m_body { body } { }

    DefNode(const Token &token, SharedPtr<String> name, Vector<Node *> &args, BlockNode *body)
        : NodeWithArgs { token, args }
        , m_name { name }
        , m_body { body } { }

    ~DefNode() {
        delete m_self_node;
        delete m_body;
    }

    virtual Type type() const override { return Type::Def; }

    Node *self_node() const { return m_self_node; }
    SharedPtr<String> name() const { return m_name; }
    BlockNode *body() const { return m_body; }

    virtual void transform(Creator *creator) const override {
        if (m_self_node) {
            creator->set_type("defs");
            creator->append(m_self_node);
        } else {
            creator->set_type("defn");
        }
        creator->append_symbol(m_name);
        append_method_or_block_args(creator);
        if (m_body->is_empty()) {
            creator->append_sexp([&](Creator *c) { c->set_type("nil"); });
        } else {
            for (auto node : m_body->nodes())
                creator->append(node);
        }
    }

protected:
    Node *m_self_node { nullptr };
    SharedPtr<String> m_name {};
    BlockNode *m_body { nullptr };
};
}
