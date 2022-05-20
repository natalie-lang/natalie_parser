#include "natalie_parser/node/interpolated_string_node.hpp"
#include "natalie_parser/node/string_node.hpp"

namespace NatalieParser {

void InterpolatedStringNode::transform(Creator *creator) const {
    creator->set_type("dstr");

    bool has_starter_string = false;
    bool only_static = true;

    for (size_t i = 0; i < m_nodes.size(); ++i) {
        auto node = m_nodes.at(i);

        // the first item is always a literal string
        if (i == 0 && node->type() == Node::Type::String) {
            auto string = node.static_cast_as<StringNode>()->string();

            // append consecutive static strings to this one
            while (i + 1 < m_nodes.size() && m_nodes.at(i + 1)->type() == Node::Type::String) {
                auto n = m_nodes[++i];
                string = new String(*string);
                string->append(*n.static_cast_as<StringNode>()->string());
            }

            creator->append_string(string);
            has_starter_string = true;

        } else {
            only_static = false;

            // if we don't have a static string to start, append a blank one
            // (this is what RubyParser does and we mimic it)
            if (!has_starter_string) {
                creator->append_string("");
                has_starter_string = true;
            }

            creator->append(node);
        }
    }

    // if we only appended static strings, then this isn't a dstr
    if (only_static)
        creator->set_type("str");
}

}
