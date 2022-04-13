#pragma once

#include "natalie_parser/node/interpolated_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class InterpolatedSymbolNode : public InterpolatedNode {
public:
    InterpolatedSymbolNode(const Token &token)
        : InterpolatedNode { token } { }

    InterpolatedSymbolNode(const InterpolatedNode &other)
        : InterpolatedNode { other } { }

    virtual Type type() const override { return Type::InterpolatedSymbol; }

    virtual void transform(Creator *creator) const override;
};
}
