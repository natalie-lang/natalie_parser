#pragma once

#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class IterNode : public NodeWithArgs {
public:
    IterNode(const Token &token, Node *call, const Vector<Node *> &args, BlockNode *body)
        : NodeWithArgs { token, args }
        , m_call { call }
        , m_body { body } {
        assert(m_call);
        assert(m_body);
    }

    IterNode(const IterNode &other)
        : NodeWithArgs { other.token() }
        , m_call { other.call().clone() }
        , m_body { new BlockNode { other.body() } } {
        for (auto arg : other.args())
            add_arg(arg->clone());
    }

    virtual Node *clone() const override {
        return new IterNode(*this);
    }

    virtual Type type() const override { return Type::Iter; }

    const Node &call() const { return m_call.ref(); }
    const BlockNode &body() const { return m_body.ref(); }

    virtual void transform(Creator *creator) const override {
        creator->set_type("iter");
        creator->append(m_call.ref());
        if (args().is_empty()) {
            creator->append_integer(0);
        } else {
            append_method_or_block_args(creator);
        }
        if (!m_body->is_empty())
            creator->append(m_body->without_unnecessary_nesting());
    }

protected:
    OwnedPtr<Node> m_call {};
    OwnedPtr<BlockNode> m_body {};
};
}
