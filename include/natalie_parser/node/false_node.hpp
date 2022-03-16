#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class FalseNode : public Node {
public:
    FalseNode(const Token &token)
        : Node { token } { }

    FalseNode(const FalseNode &other)
        : FalseNode { other.token() } { }

    virtual Node *clone() const override {
        return new FalseNode(*this);
    }

    virtual Type type() const override { return Type::False; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("false");
    }
};
}
