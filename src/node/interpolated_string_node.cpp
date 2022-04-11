#include "natalie_parser/node/interpolated_string_node.hpp"
#include "natalie_parser/node/string_node.hpp"

namespace NatalieParser {

void InterpolatedStringNode::transform(Creator *creator) const {
    creator->set_type("dstr");
    bool has_starter_string = false;
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        auto node = m_nodes.at(i);
        if (i == 0 && node->type() == Node::Type::String) {
            creator->append_string(static_cast<StringNode *>(node)->string());
            has_starter_string = true;
        } else {
            if (!has_starter_string) {
                creator->append_string("");
                has_starter_string = true;
            }
            creator->append(node);
        }
    }
}

}
