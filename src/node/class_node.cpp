#include "natalie_parser/node/class_node.hpp"
#include "natalie_parser/node/identifier_node.hpp"

namespace NatalieParser {

void ClassNode::transform(Creator *creator) const {
    creator->set_type("class");
    auto doc_comment = doc();
    if (doc_comment)
        creator->set_comments(doc_comment.value().ref());
    if (m_name->type() == Node::Type::Identifier) {
        auto identifier = m_name.static_cast_as<IdentifierNode>();
        creator->append_symbol(identifier->name());
    } else {
        creator->append(m_name.ref());
    }
    creator->append(m_superclass.ref());
    for (auto node : m_body->nodes())
        creator->append(node);
}

}
