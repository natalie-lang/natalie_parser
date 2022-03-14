#include "natalie_parser/node/node_with_args.hpp"
#include "natalie_parser/node/arg_node.hpp"

namespace NatalieParser {

void NodeWithArgs::append_method_or_block_args(Creator *creator) const {
    creator->append_sexp([&](Creator *c) {
        c->set_type("args");
        for (auto arg : m_args) {
            switch (arg->type()) {
            case Node::Type::Arg: {
                auto arg_node = static_cast<ArgNode *>(arg);
                if (arg_node->value())
                    c->append(arg);
                else
                    static_cast<ArgNode *>(arg)->append_name(c);
                break;
            }
            case Node::Type::KeywordArg:
            case Node::Type::MultipleAssignmentArg:
                c->append(arg);
                break;
            default:
                TM_UNREACHABLE();
            }
        }
    });
}

}
