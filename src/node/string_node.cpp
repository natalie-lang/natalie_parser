#include "natalie_parser/node/string_node.hpp"
#include "natalie_parser/node/interpolated_string_node.hpp"

namespace NatalieParser {

SharedPtr<Node> StringNode::append_string_node(SharedPtr<Node> string2) const {
    switch (string2->type()) {
    case Node::Type::String: {
        auto string2_node = string2.static_cast_as<StringNode>();
        SharedPtr<String> str = new String { *m_string };
        str->append(*string2_node->string());
        return new StringNode { m_token, str };
    }
    case Node::Type::InterpolatedString: {
        auto string2_node = string2.static_cast_as<InterpolatedStringNode>();
        assert(!string2_node->is_empty());
        if (string2_node->nodes().first()->type() == Node::Type::String) {
            auto n1 = string2_node->nodes().first().static_cast_as<StringNode>();
            n1->string()->prepend(*m_string);
        } else {
            auto copy = new StringNode(m_token, m_string);
            string2_node->prepend_node(copy);
        }
        return string2;
    }
    default:
        TM_UNREACHABLE();
    }
}

}
