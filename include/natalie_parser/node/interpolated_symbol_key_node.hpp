#pragma once

#include "natalie_parser/node/interpolated_symbol_node.hpp"

namespace NatalieParser {

using namespace TM;

class InterpolatedSymbolKeyNode : public InterpolatedSymbolNode {
public:
    InterpolatedSymbolKeyNode(const InterpolatedNode &other)
        : InterpolatedSymbolNode { other } { }

    virtual Type type() const override { return Type::InterpolatedSymbolKey; }
};
}
