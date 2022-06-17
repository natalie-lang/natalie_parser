#include "natalie_parser/node.hpp"
#include "natalie_parser/creator/debug_creator.hpp"

namespace NatalieParser {

BlockNode &Node::as_block_node() {
    assert(type() == Node::Type::Block);
    return *static_cast<BlockNode *>(this);
}

void Node::debug() {
    DebugCreator creator;
    transform(&creator);
    printf("DEBUG[type=%d]: %s\n", (int)type(), creator.to_string().c_str());
}

}
