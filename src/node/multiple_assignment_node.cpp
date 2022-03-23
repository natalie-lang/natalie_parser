#include "natalie_parser/node/multiple_assignment_node.hpp"
#include "natalie_parser/node/identifier_node.hpp"
#include "natalie_parser/node/splat_node.hpp"

namespace NatalieParser {

void MultipleAssignmentNode::add_locals(TM::Hashmap<TM::String> &locals) {
    for (auto node : m_nodes) {
        switch (node->type()) {
        case Node::Type::Identifier: {
            auto identifier = static_cast<IdentifierNode *>(node);
            identifier->add_to_locals(locals);
            break;
        }
        case Node::Type::Call:
        case Node::Type::Colon2:
        case Node::Type::Colon3:
            break;
        case Node::Type::Splat: {
            auto splat = static_cast<SplatNode *>(node);
            if (splat->node() && splat->node().type() == Node::Type::Identifier) {
                auto identifier = static_cast<const IdentifierNode *>(&splat->node());
                identifier->add_to_locals(locals);
            }
            break;
        }
        case Node::Type::MultipleAssignment:
            static_cast<MultipleAssignmentNode *>(node)->add_locals(locals);
            break;
        default:
            TM_UNREACHABLE();
        }
    }
}

}
