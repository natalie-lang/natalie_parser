#pragma once

#include "natalie_parser/node/interpolated_string_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "natalie_parser/node/symbol_node.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class StringNode : public Node {
public:
    StringNode(const Token &token, SharedPtr<String> string)
        : Node { token }
        , m_string { string } {
        assert(m_string);
    }

    virtual Type type() const override { return Type::String; }

    virtual bool can_be_concatenated_to_a_string() const override { return true; }

    SharedPtr<String> string() const { return m_string; }

    SharedPtr<SymbolNode> to_symbol_node() const {
        return new SymbolNode { m_token, m_string };
    }

    SharedPtr<Node> append_string_node(SharedPtr<Node> string2) const;

    virtual void transform(Creator *creator) const override {
        creator->set_type("str");
        creator->append_string(m_string);
    }

protected:
    SharedPtr<String> m_string {};
};
}
