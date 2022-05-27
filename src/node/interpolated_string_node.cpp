#include "natalie_parser/node/interpolated_string_node.hpp"
#include "natalie_parser/node/evaluate_to_string_node.hpp"
#include "natalie_parser/node/string_node.hpp"

namespace NatalieParser {

SharedPtr<Node> InterpolatedStringNode::append_string_node(SharedPtr<Node> string2) const {
    SharedPtr<InterpolatedStringNode> copy = new InterpolatedStringNode { *this };
    switch (string2->type()) {
    case Node::Type::String: {
        auto string2_node = string2.static_cast_as<StringNode>();
        assert(!is_empty());
        auto last_node = copy->nodes().last();
        switch (last_node->type()) {
        case Node::Type::String:
            if (m_nodes.size() > 1) {
                // For some reason, RubyParser doesn't append two string nodes
                // if there is an evstr present.
                copy->add_node(string2);
            } else {
                // NOTE: This mutates one of my own nodes, but I don't care.
                last_node.static_cast_as<StringNode>()->string()->append(*string2_node->string());
            }
            break;
        case Node::Type::EvaluateToString:
            copy->add_node(string2);
            break;
        default:
            TM_UNREACHABLE();
        }
        return copy.static_cast_as<Node>();
    }
    case Node::Type::InterpolatedString: {
        auto string2_node = string2.static_cast_as<InterpolatedStringNode>();
        assert(!is_empty());
        assert(!string2_node->is_empty());
        for (auto node : string2_node->nodes()) {
            copy->add_node(node);
        }
        return copy.static_cast_as<Node>();
    }
    default:
        TM_UNREACHABLE();
    }
}

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

            // if this node is an InterpolatedString having a single
            // EvaluateToString node, then yoink the EvaluateToString
            // for ourselves, and don't bother with the useless wrapper
            //
            //     # bad
            //     s(:dstr, "", s(:evstr, s(:dstr, "", s(:evstr, s(:lit, 1)))))
            //
            //     # good
            //     s(:dstr, "", s(:evstr, s(:lit, 1)))
            //
            if (node->type() == Node::Type::EvaluateToString) {
                auto evstr = node.static_cast_as<EvaluateToStringNode>();
                if (evstr->node() && evstr->node()->type() == Node::Type::InterpolatedString) {
                    auto dstr = evstr->node().static_cast_as<InterpolatedStringNode>();
                    if (dstr->nodes().size() == 1 && dstr->nodes().first()->type() == Node::Type::EvaluateToString) {
                        auto n1 = dstr->nodes().first().static_cast_as<EvaluateToStringNode>();
                        creator->append(dstr->nodes().first());
                        continue;
                    }
                }
            }

            creator->append(node);
        }
    }

    // if we only appended static strings, then this isn't a dstr
    if (only_static)
        creator->set_type("str");
}

}
