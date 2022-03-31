#include "natalie_parser/node.hpp"

namespace NatalieParser {

NodeWithArgs *Node::to_node_with_args() {
    switch (type()) {
    case Node::Type::Identifier: {
        auto identifier = static_cast<IdentifierNode *>(this);
        auto call_node = new CallNode {
            identifier->token(),
            new NilNode { identifier->token() },
            identifier->name(),
        };
        delete identifier;
        return call_node;
    }
    case Node::Type::Call:
        return static_cast<CallNode *>(this);
    case Node::Type::SafeCall:
        return static_cast<SafeCallNode *>(this);
    case Node::Type::Super:
        return static_cast<SuperNode *>(this);
    case Node::Type::Undef:
        return static_cast<UndefNode *>(this);
    case Node::Type::Yield:
        return static_cast<YieldNode *>(this);
    default:
        TM_UNREACHABLE();
    }
}

}
