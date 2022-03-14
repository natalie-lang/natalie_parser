#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class ModuleNode : public Node {
public:
    ModuleNode(const Token &token, Node *name, BlockNode *body)
        : Node { token }
        , m_name { name }
        , m_body { body } { }

    ~ModuleNode() {
        delete m_name;
        delete m_body;
    }

    virtual Type type() const override { return Type::Module; }

    Node *name() const { return m_name; }
    BlockNode *body() const { return m_body; }

    virtual void transform(Creator *creator) const override;

protected:
    Node *m_name { nullptr };
    BlockNode *m_body { nullptr };
};
}
