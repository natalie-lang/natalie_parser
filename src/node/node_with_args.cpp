#include "natalie_parser/node/node_with_args.hpp"
#include "natalie_parser/node/arg_node.hpp"
#include "natalie_parser/node/shadow_arg_node.hpp"

namespace NatalieParser {

void NodeWithArgs::append_method_or_block_args(Creator *creator) const {
    creator->append_sexp([&](Creator *c) {
        c->set_type("args");
        for (auto arg : m_args) {
            switch (arg->type()) {
            case Node::Type::Arg: {
                auto arg_node = arg.static_cast_as<ArgNode>();
                if (arg_node->value())
                    c->append(arg);
                else
                    arg.static_cast_as<ArgNode>()->append_name(c);
                break;
            }
            case Node::Type::KeywordArg:
            case Node::Type::MultipleAssignmentArg:
            case Node::Type::ShadowArg:
                c->append(arg);
                break;
            case Node::Type::Nil:
                c->append_nil();
                break;
            default:
                TM_UNREACHABLE();
            }
        }
    });
}

}
