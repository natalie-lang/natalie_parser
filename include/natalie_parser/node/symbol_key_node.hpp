#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/symbol_node.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class SymbolKeyNode : public SymbolNode {
public:
    using SymbolNode::SymbolNode;

    virtual Type type() const override { return Type::SymbolKey; }

    virtual bool is_symbol_key() const override { return true; }
};
}
