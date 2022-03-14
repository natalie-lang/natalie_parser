#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/array_node.hpp"
#include "natalie_parser/node/arg_node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class MultipleAssignmentArgNode : public ArrayNode {
public:
    MultipleAssignmentArgNode(const Token &token)
        : ArrayNode { token } { }

    virtual Type type() const override { return Type::MultipleAssignmentArg; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("masgn");
        for (auto arg : nodes()) {
            if (arg->type() == Node::Type::Arg) {
                static_cast<ArgNode *>(arg)->append_name(creator);
            } else {
                creator->append(arg);
            }
        }
    }
};
}
