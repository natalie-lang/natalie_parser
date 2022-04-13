#pragma once

#include "natalie_parser/node/node.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class SymbolKeyNode : public SymbolNode {
public:
    using SymbolNode::SymbolNode;

    SymbolKeyNode(const SymbolKeyNode &other)
        : SymbolKeyNode { other.token(), other.name() } { }

    virtual Node *clone() const override {
        return new SymbolKeyNode(*this);
    }

    virtual Type type() const override { return Type::SymbolKey; }
};
}
