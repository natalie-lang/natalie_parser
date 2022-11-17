#include "natalie_parser/node/begin_rescue_node.hpp"
#include "natalie_parser/node/array_node.hpp"
#include "natalie_parser/node/assignment_node.hpp"
#include "natalie_parser/node/identifier_node.hpp"

namespace NatalieParser {

SharedPtr<Node> BeginRescueNode::name_to_assignment() const {
    assert(m_name);
    return new AssignmentNode {
        token(),
        m_name,
        new IdentifierNode {
            Token { Token::Type::GlobalVariable, "$!", file(), line(), column(), false },
            false },
    };
}

void BeginRescueNode::transform(Creator *creator) const {
    creator->set_type("resbody");
    auto array = ArrayNode { token() };
    for (auto exception_node : m_exceptions)
        array.add_node(exception_node);
    if (m_name)
        array.add_node(name_to_assignment());
    creator->append(array);
    if (m_body->nodes().is_empty())
        creator->append_nil();
    else
        for (auto node : m_body->nodes())
            creator->append(node);
}

}
