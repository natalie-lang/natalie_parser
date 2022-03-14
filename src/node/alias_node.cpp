#include "natalie_parser/node/alias_node.hpp"

namespace NatalieParser {

AliasNode::~AliasNode() {
    delete m_new_name;
    delete m_existing_name;
}

void AliasNode::transform(Creator *creator) const {
    creator->set_type("alias");
    creator->append(m_new_name);
    creator->append(m_existing_name);
}

}
