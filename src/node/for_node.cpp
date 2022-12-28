#include "natalie_parser/node/for_node.hpp"

namespace NatalieParser {

void ForNode::transform(Creator *creator) const {
    creator->set_type("for");
    creator->append(m_expr);
    creator->with_assignment(true, [&]() { creator->append(*m_vars); });
    if (!m_body->is_empty())
        creator->append(m_body->without_unnecessary_nesting());
}

}
