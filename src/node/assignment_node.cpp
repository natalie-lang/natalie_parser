#include "natalie_parser/node/assignment_node.hpp"
#include "natalie_parser/node/multiple_assignment_node.hpp"

namespace NatalieParser {

void AssignmentNode::transform(Creator *creator) const {
    switch (identifier().type()) {
    case Node::Type::MultipleAssignment: {
        auto masgn = static_cast<const MultipleAssignmentNode *>(&identifier());
        masgn->transform(creator);
        creator->append(m_value.ref());
        break;
    }
    case Node::Type::Call:
    case Node::Type::Colon2:
    case Node::Type::Colon3:
    case Node::Type::Identifier: {
        creator->with_assignment(true, [&]() {
            identifier().transform(creator);
        });
        creator->append(m_value.ref());
        break;
    }
    default:
        TM_UNREACHABLE();
    }
}

}
