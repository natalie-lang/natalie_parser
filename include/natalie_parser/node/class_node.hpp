#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class ClassNode : public Node {
public:
    ClassNode(const Token &token, Node *name, Node *superclass, BlockNode *body)
        : Node { token }
        , m_name { name }
        , m_superclass { superclass }
        , m_body { body } {
        assert(m_name);
        assert(m_superclass);
        assert(m_body);
    }

    ~ClassNode();

    virtual Type type() const override { return Type::Class; }

    Node *name() const { return m_name; }
    Node *superclass() const { return m_superclass; }
    BlockNode *body() const { return m_body; }

    virtual void transform(Creator *creator) const override;

protected:
    Node *m_name { nullptr };
    Node *m_superclass { nullptr };
    BlockNode *m_body { nullptr };
};
}
