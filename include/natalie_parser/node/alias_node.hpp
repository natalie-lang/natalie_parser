#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "natalie_parser/node/symbol_node.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class AliasNode : public Node {
public:
    AliasNode(const Token &token, SymbolNode *new_name, SymbolNode *existing_name)
        : Node { token }
        , m_new_name { new_name }
        , m_existing_name { existing_name } { }

    ~AliasNode();

    virtual Type type() const override { return Type::Alias; }

    SymbolNode *new_name() const { return m_new_name; }
    SymbolNode *existing_name() const { return m_existing_name; }

    virtual void transform(Creator *creator) const override;

private:
    SymbolNode *m_new_name { nullptr };
    SymbolNode *m_existing_name { nullptr };
};
}
