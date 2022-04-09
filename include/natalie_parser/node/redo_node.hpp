#pragma once

#include "natalie_parser/node/node.hpp"

namespace NatalieParser {

using namespace TM;

class RedoNode : public Node {
public:
    RedoNode(const Token &token)
        : Node { token } { }

    virtual Type type() const override { return Type::Redo; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("redo");
    }
};
}
