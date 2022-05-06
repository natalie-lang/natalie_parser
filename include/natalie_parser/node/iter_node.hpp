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
    IterNode(const Token &token, SharedPtr<Node> call, bool has_args, const Vector<SharedPtr<Node>> &args, SharedPtr<BlockNode> body)
        : NodeWithArgs { token, args }
        , m_has_args { has_args }
        , m_call { call }
        , m_body { body } {
        assert(m_call);
        assert(m_body);
    }

    virtual Type type() const override { return Type::Iter; }

    const SharedPtr<Node> call() const { return m_call; }
    const SharedPtr<BlockNode> body() const { return m_body; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("iter");
        creator->append(m_call.ref());
        if (m_has_args)
            append_method_or_block_args(creator);
        else
            creator->append_integer(0);
        if (!m_body->is_empty())
            creator->append(m_body->without_unnecessary_nesting());
    }

protected:
    bool m_has_args { false };
    SharedPtr<Node> m_call {};
    SharedPtr<BlockNode> m_body {};
};
}
