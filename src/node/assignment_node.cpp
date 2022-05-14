#include "natalie_parser/node/assignment_node.hpp"
#include "natalie_parser/node/multiple_assignment_node.hpp"

namespace NatalieParser {

void AssignmentNode::transform(Creator *creator) const {
    creator->set_line(m_identifier->line());
    creator->set_column(m_identifier->column());
    creator->reset_sexp();
    switch (m_identifier->type()) {
    case Node::Type::MultipleAssignment: {
        auto masgn = m_identifier;
        masgn->transform(creator);
        creator->append(m_value.ref());
        break;
    }
    case Node::Type::Call:
    case Node::Type::Colon2:
    case Node::Type::Colon3:
    case Node::Type::Identifier:
    case Node::Type::SafeCall: {
        creator->with_assignment(true, [&]() {
            m_identifier->transform(creator);
        });
        creator->append(m_value.ref());
        break;
    }
    default:
        TM_UNREACHABLE();
    }
}

}
