#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class TrueNode : public Node {
public:
    TrueNode(const Token &token)
        : Node { token } { }

    virtual Type type() const override { return Type::True; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("true");
    }
};
}
