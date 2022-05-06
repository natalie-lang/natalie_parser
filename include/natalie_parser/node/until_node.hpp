#pragma once

#include "natalie_parser/node/block_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "natalie_parser/node/while_node.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class UntilNode : public WhileNode {
public:
    UntilNode(const Token &token, SharedPtr<Node> condition, SharedPtr<BlockNode> body, bool pre)
        : WhileNode { token, condition, body, pre } { }

    virtual Type type() const override { return Type::Until; }
};
}
