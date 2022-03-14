#include "natalie_parser/node/module_node.hpp"
#include "natalie_parser/node/identifier_node.hpp"

namespace NatalieParser {

void ModuleNode::transform(Creator *creator) const {
    creator->set_type("module");
    if (m_name->type() == Node::Type::Identifier) {
        auto identifier = static_cast<IdentifierNode *>(m_name);
        creator->append_symbol(identifier->name());
    } else {
        creator->append(m_name);
    }
    for (auto node : m_body->nodes())
        creator->append(node);
}

}
