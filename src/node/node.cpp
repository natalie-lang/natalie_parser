#include "natalie_parser/node.hpp"

namespace NatalieParser {

BlockNode &Node::as_block_node() {
    assert(type() == Node::Type::Block);
    return *static_cast<BlockNode *>(this);
}

}
