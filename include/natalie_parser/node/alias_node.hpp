#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "natalie_parser/node/symbol_node.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class AliasNode : public Node {
public:
    AliasNode(const Token &token, SymbolNode *new_name, SymbolNode *existing_name)
        : Node { token }
        , m_new_name { new_name }
        , m_existing_name { existing_name } {
        assert(m_new_name);
        assert(m_existing_name);
    }

    virtual Type type() const override { return Type::Alias; }

    const SymbolNode &new_name() const { return m_new_name.ref(); }
    const SymbolNode &existing_name() const { return m_existing_name.ref(); }

    virtual void transform(Creator *creator) const override;

private:
    OwnedPtr<SymbolNode> m_new_name {};
    OwnedPtr<SymbolNode> m_existing_name {};
};
}
