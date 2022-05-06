#pragma once

#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class DefNode : public NodeWithArgs {
public:
    DefNode(const Token &token, SharedPtr<Node> self_node, SharedPtr<String> name, const Vector<SharedPtr<Node>> &args, SharedPtr<BlockNode> body)
        : NodeWithArgs { token, args }
        , m_self_node { self_node }
        , m_name { name }
        , m_body { body } {
        assert(m_body);
    }

    virtual Type type() const override { return Type::Def; }

    const SharedPtr<Node> self_node() const { return m_self_node; }
    SharedPtr<String> name() const { return m_name; }
    const SharedPtr<BlockNode> body() const { return m_body; }

    virtual void transform(Creator *creator) const override {
        if (m_self_node) {
            creator->set_type("defs");
            creator->append(m_self_node.ref());
        } else {
            creator->set_type("defn");
        }
        auto doc_comment = doc();
        if (doc_comment)
            creator->set_comments(doc_comment.value().ref());
        creator->append_symbol(m_name);
        append_method_or_block_args(creator);
        if (m_body->is_empty()) {
            creator->append_nil_sexp();
        } else {
            for (auto node : m_body->nodes())
                creator->append(node);
        }
    }

protected:
    SharedPtr<Node> m_self_node {};
    SharedPtr<String> m_name {};
    SharedPtr<BlockNode> m_body {};
};
}
