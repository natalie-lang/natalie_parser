#include "natalie_parser/node/module_node.hpp"
#include "natalie_parser/node/identifier_node.hpp"

namespace NatalieParser {

void ModuleNode::transform(Creator *creator) const {
    creator->set_type("module");
    auto doc_comment = doc();
    if (doc_comment)
        creator->set_comments(doc_comment.value().ref());
    if (m_name->type() == Node::Type::Identifier) {
        auto identifier = static_cast<const IdentifierNode *>(&m_name.ref());
        creator->append_symbol(identifier->name());
    } else {
        creator->append(m_name.ref());
    }
    for (auto node : m_body->nodes())
        creator->append(node);
}

}
