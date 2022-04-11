#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class SymbolNode : public Node {
public:
    SymbolNode(const Token &token, SharedPtr<String> name)
        : Node { token }
        , m_name { name } { }

    SymbolNode(const SymbolNode &other)
        : SymbolNode { other.token(), other.name() } { }

    virtual Node *clone() const override {
        return new SymbolNode(*this);
    }

    virtual Type type() const override { return Type::Symbol; }

    SharedPtr<String> name() const { return m_name; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("lit");
        creator->append_symbol(m_name);
    }

protected:
    SharedPtr<String> m_name {};
};
}
