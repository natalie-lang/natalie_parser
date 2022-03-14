#include "natalie_parser/node/interpolated_regexp_node.hpp"
#include "natalie_parser/node/string_node.hpp"

namespace NatalieParser {

void InterpolatedRegexpNode::transform(Creator *creator) const {
    creator->set_type("dregx");
    for (size_t i = 0; i < nodes().size(); ++i) {
        auto node = nodes()[i];
        if (i == 0 && node->type() == Node::Type::String)
            creator->append_string(static_cast<StringNode *>(node)->string());
        else
            creator->append(node);
    }
    if (m_options != 0)
        creator->append_integer(m_options);
}

}
