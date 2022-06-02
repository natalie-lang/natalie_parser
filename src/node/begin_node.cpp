#include "natalie_parser/node/begin_node.hpp"

namespace NatalieParser {

void BeginNode::transform(Creator *creator) const {
    assert(m_body);
    creator->set_type("rescue");
    if (!m_body->is_empty())
        creator->append(m_body->without_unnecessary_nesting());
    for (auto rescue_node : m_rescue_nodes) {
        creator->append(rescue_node.static_cast_as<Node>());
    }
    if (m_else_body) {
        if (!m_else_body->is_empty())
            creator->append(m_else_body->without_unnecessary_nesting());
    }
    if (m_ensure_body) {
        if (m_rescue_nodes.is_empty())
            creator->set_type("ensure");
        else
            creator->wrap("ensure");
        if (m_ensure_body->is_empty())
            creator->append_nil_sexp();
        else
            creator->append(m_ensure_body->without_unnecessary_nesting());
    }
}

}
