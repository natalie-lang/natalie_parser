#pragma once

#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class SclassNode : public Node {
public:
    SclassNode(const Token &token, Node *klass, BlockNode *body)
        : Node { token }
        , m_klass { klass }
        , m_body { body } { }

    ~SclassNode() {
        delete m_klass;
        delete m_body;
    }

    virtual Type type() const override { return Type::Sclass; }

    Node *klass() const { return m_klass; }
    BlockNode *body() const { return m_body; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("sclass");
        creator->append(m_klass);
        for (auto node : m_body->nodes())
            creator->append(node);
    }

protected:
    Node *m_klass { nullptr };
    BlockNode *m_body { nullptr };
};
}
