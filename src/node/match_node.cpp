#include "natalie_parser/node/match_node.hpp"

namespace NatalieParser {

MatchNode::~MatchNode() {
    delete m_regexp;
    delete m_arg;
}

void MatchNode::transform(Creator *creator) const {
    if (m_regexp_on_left)
        creator->set_type("match2");
    else
        creator->set_type("match3");
    creator->append(m_regexp);
    creator->append(m_arg);
}

}
