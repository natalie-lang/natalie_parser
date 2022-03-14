#pragma once

#include "natalie_parser/node/arg_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class KeywordArgNode : public ArgNode {
public:
    KeywordArgNode(const Token &token, SharedPtr<String> name)
        : ArgNode { token, name } { }

    virtual Type type() const override { return Type::KeywordArg; }

    virtual void transform(Creator *creator) const override {
        ArgNode::transform(creator);
        creator->set_type("kwarg");
    }
};
}
