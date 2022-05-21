#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class EndBlockNode : public Node {
public:
    EndBlockNode(const Token &token)
        : Node { token } { }

    virtual Type type() const override { return Type::EndBlock; }

    virtual bool can_accept_a_block() const override { return true; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("postexe");
    }
};
}
