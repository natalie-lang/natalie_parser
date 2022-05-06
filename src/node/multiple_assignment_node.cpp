#include "natalie_parser/node/multiple_assignment_node.hpp"
#include "natalie_parser/node/identifier_node.hpp"
#include "natalie_parser/node/splat_node.hpp"

namespace NatalieParser {

void MultipleAssignmentNode::add_locals(TM::Hashmap<TM::String> &locals) {
    for (auto node : m_nodes) {
        switch (node->type()) {
        case Node::Type::Identifier: {
            auto identifier = node.static_cast_as<IdentifierNode>();
            identifier->add_to_locals(locals);
            break;
        }
        case Node::Type::Call:
        case Node::Type::Colon2:
        case Node::Type::Colon3:
            break;
        case Node::Type::Splat: {
            auto splat = node.static_cast_as<SplatNode>();
            if (splat->node() && splat->node()->type() == Node::Type::Identifier) {
                auto identifier = splat->node().static_cast_as<IdentifierNode>();
                identifier->add_to_locals(locals);
            }
            break;
        }
        case Node::Type::MultipleAssignment:
            node.static_cast_as<MultipleAssignmentNode>()->add_locals(locals);
            break;
        default:
            printf("unknown node type %d\n", (int)node->type());
            TM_UNREACHABLE();
        }
    }
}

}
