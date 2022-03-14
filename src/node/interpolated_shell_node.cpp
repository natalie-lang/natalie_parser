#include "natalie_parser/node/interpolated_shell_node.hpp"
#include "natalie_parser/node/string_node.hpp"

namespace NatalieParser {

void InterpolatedShellNode::transform(Creator *creator) const {
    creator->set_type("dxstr");
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        auto node = m_nodes.at(i);
        if (i == 0 && node->type() == Node::Type::String) {
            auto string_node = static_cast<StringNode *>(node);
            creator->append_string(string_node->string());
        } else {
            creator->append(node);
        }
    }
}

}
