#pragma once

#include "natalie_parser/node/call_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class SafeCallNode : public CallNode {
public:
    SafeCallNode(const Token &token, Node *receiver, SharedPtr<String> message)
        : CallNode { token, receiver, message } { }

    SafeCallNode(const Token &token, CallNode &node)
        : CallNode { token, node } { }

    virtual Type type() const override { return Type::SafeCall; }

    virtual void transform(Creator *creator) const override {
        CallNode::transform(creator);
        creator->set_type("safe_call");
    }
};
}
