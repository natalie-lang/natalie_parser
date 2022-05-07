#include "natalie_parser/node/op_assign_node.hpp"
#include "natalie_parser/node/identifier_node.hpp"

namespace NatalieParser {

void OpAssignNode::transform(Creator *creator) const {
    assert(m_op);
    switch (m_name->type()) {
    case Node::Type::Identifier: {
        auto identifier_node = m_name.static_cast_as<IdentifierNode>();
        creator->with_assignment(true, [&]() {
            identifier_node->transform(creator);
        });
        auto value = CallNode {
            token(),
            m_name.static_cast_as<Node>(),
            m_op
        };
        value.add_arg(m_value);
        creator->append(value);
        break;
    }
    case Node::Type::Colon2:
    case Node::Type::Colon3:
        creator->set_type("op_asgn");
        creator->append(m_name);
        creator->append_symbol(m_op);
        creator->append(m_value);
        break;
    default:
        printf("got node type %d\n", (int)m_name->type());
        TM_UNREACHABLE();
    }
}

}
