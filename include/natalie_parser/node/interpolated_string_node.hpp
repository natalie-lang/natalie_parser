#pragma once

#include "natalie_parser/node/interpolated_node.hpp"
#include "natalie_parser/node/interpolated_symbol_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class InterpolatedStringNode : public InterpolatedNode {
public:
    InterpolatedStringNode(const Token &token)
        : InterpolatedNode { token } { }

    virtual Type type() const override { return Type::InterpolatedString; }

    virtual bool can_be_concatenated_to_a_string() const override { return true; }

    SharedPtr<InterpolatedSymbolNode> to_symbol_node() const {
        return new InterpolatedSymbolNode { *this };
    }

    SharedPtr<Node> append_string_node(SharedPtr<Node> string2) const;

    virtual void transform(Creator *creator) const override;
};
}
