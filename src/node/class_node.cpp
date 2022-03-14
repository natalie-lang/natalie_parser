#include "natalie_parser/node/class_node.hpp"
#include "natalie_parser/node/identifier_node.hpp"

namespace NatalieParser {

ClassNode::~ClassNode() {
    delete m_name;
    delete m_superclass;
    delete m_body;
}

void ClassNode::transform(Creator *creator) const {
    creator->set_type("class");
    if (m_name->type() == Node::Type::Identifier) {
        auto identifier = static_cast<IdentifierNode *>(m_name);
        creator->append_symbol(identifier->name());
    } else {
        creator->append(m_name);
    }
    creator->append(m_superclass);
    for (auto node : m_body->nodes())
        creator->append(node);
}

}
