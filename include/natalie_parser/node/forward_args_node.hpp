#pragma once

#include "natalie_parser/node/node.hpp"
#include "tm/hashmap.hpp"

namespace NatalieParser {

using namespace TM;

class ForwardArgsNode : public Node {
public:
    ForwardArgsNode(const Token &token)
        : Node { token } { }

    virtual Type type() const override { return Type::ForwardArgs; }

    void add_to_locals(TM::Hashmap<TM::String> &locals) {
        locals.set("...");
    }

    virtual void transform(Creator *creator) const override {
        creator->set_type("forward_args");
    }
};

}
