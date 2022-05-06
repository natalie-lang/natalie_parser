#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class UndefNode : public NodeWithArgs {
public:
    UndefNode(const Token &token)
        : NodeWithArgs { token } { }

    virtual Type type() const override { return Type::Undef; }

    // NOTE: UndefNode is handled separately from yield, break, etc.,
    // so for our purposes here, it is not "callable".
    virtual bool is_callable() const override { return false; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("undef");
        if (args().is_empty())
            return;
        for (auto arg : args())
            creator->append(arg);
    }
};
}
