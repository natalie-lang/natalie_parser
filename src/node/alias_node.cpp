#include "natalie_parser/node/alias_node.hpp"

namespace NatalieParser {

void AliasNode::transform(Creator *creator) const {
    creator->set_type("alias");
    creator->append(m_new_name.ref());
    creator->append(m_existing_name.ref());
}

}
