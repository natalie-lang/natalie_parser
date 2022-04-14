#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class OpAssignAccessorNode : public NodeWithArgs {
public:
    OpAssignAccessorNode(const Token &token, SharedPtr<String> op, Node *receiver, SharedPtr<String> message, Node *value, Vector<Node *> &args)
        : NodeWithArgs { token }
        , m_op { op }
        , m_receiver { receiver }
        , m_message { message }
        , m_value { value } {
        assert(m_op);
        assert(m_receiver);
        assert(m_message);
        assert(m_value);
        for (auto arg : args)
            add_arg(arg->clone());
    }

    virtual Type type() const override { return Type::OpAssignAccessor; }

    SharedPtr<String> op() const { return m_op; }
    const Node &receiver() const { return m_receiver.ref(); }
    SharedPtr<String> message() const { return m_message; }
    const Node &value() const { return m_value.ref(); }

    virtual void transform(Creator *creator) const override {
        if (*m_message == "[]=") {
            creator->set_type("op_asgn1");
            creator->append(m_receiver.ref());
            creator->append_sexp([&](Creator *c) {
                c->set_type("arglist");
                for (auto arg : args())
                    c->append(arg);
            });
            creator->append_symbol(m_op);
            creator->append(m_value.ref());
            return;
        }
        assert(args().is_empty());
        creator->set_type("op_asgn2");
        creator->append(m_receiver.ref());
        creator->append_symbol(m_message);
        creator->append_symbol(m_op);
        creator->append(m_value.ref());
    }

protected:
    SharedPtr<String> m_op {};
    OwnedPtr<Node> m_receiver {};
    SharedPtr<String> m_message {};
    OwnedPtr<Node> m_value {};
};
}
