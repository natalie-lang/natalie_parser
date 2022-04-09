#pragma once

#include "natalie_parser/node/node.hpp"

namespace NatalieParser {

using namespace TM;

class RetryNode : public Node {
public:
    RetryNode(const Token &token)
        : Node { token } { }

    virtual Type type() const override { return Type::Retry; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("retry");
    }
};
}
