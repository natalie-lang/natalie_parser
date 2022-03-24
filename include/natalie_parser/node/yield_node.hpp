#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class YieldNode : public NodeWithArgs {
public:
    YieldNode(const Token &token)
        : NodeWithArgs { token } { }

    YieldNode(const YieldNode &other)
        : NodeWithArgs { other } { }

    virtual Node *clone() const override {
        return new YieldNode(*this);
    }

    virtual Type type() const override { return Type::Yield; }

    virtual bool is_callable() const override { return true; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("yield");
        if (args().is_empty())
            return;
        for (auto arg : args())
            creator->append(arg);
    }
};
}
