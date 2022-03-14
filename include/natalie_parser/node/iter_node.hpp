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
    IterNode(const Token &token, Node *call, Vector<Node *> &args, BlockNode *body)
        : NodeWithArgs { token, args }
        , m_call { call }
        , m_body { body } {
        assert(m_call);
        assert(m_body);
    }

    ~IterNode() {
        delete m_call;
        delete m_body;
    }

    virtual Type type() const override { return Type::Iter; }

    Node *call() const { return m_call; }
    BlockNode *body() const { return m_body; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("iter");
        creator->append(m_call);
        if (args().is_empty()) {
            creator->append_integer(0);
        } else {
            append_method_or_block_args(creator);
        }
        if (!m_body->is_empty()) {
            if (m_body->has_one_node())
                creator->append(m_body->nodes()[0]);
            else
                creator->append(m_body);
        }
    }

protected:
    Node *m_call { nullptr };
    BlockNode *m_body { nullptr };
};
}
