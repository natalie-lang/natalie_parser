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
    OpAssignAccessorNode(const Token &token, SharedPtr<String> op, SharedPtr<Node> receiver, SharedPtr<String> message, SharedPtr<Node> value, Vector<SharedPtr<Node>> &args)
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
            add_arg(arg);
    }

    virtual Type type() const override { return Type::OpAssignAccessor; }

    const SharedPtr<String> op() const { return m_op; }
    const SharedPtr<Node> receiver() const { return m_receiver; }
    const SharedPtr<String> message() const { return m_message; }
    const SharedPtr<Node> value() const { return m_value; }

    bool safe() const { return m_safe; }
    void set_safe(bool safe) { m_safe = safe; }

    virtual void transform(Creator *creator) const override {
        if (*m_message == "[]=") {
            creator->set_type("op_asgn1");
            creator->append(m_receiver.ref());
            if (m_args.is_empty()) {
                creator->append_nil();
            } else {
                creator->append_sexp([&](Creator *c) {
                    c->set_type("arglist");
                    for (auto arg : m_args)
                        c->append(arg);
                });
            }
            creator->append_symbol(m_op);
            creator->append(m_value.ref());
            return;
        }
        assert(args().is_empty());
        if (m_safe)
            creator->set_type("safe_op_asgn2");
        else
            creator->set_type("op_asgn2");
        creator->append(m_receiver.ref());
        creator->append_symbol(m_message);
        creator->append_symbol(m_op);
        creator->append(m_value.ref());
    }

protected:
    SharedPtr<String> m_op {};
    SharedPtr<Node> m_receiver {};
    SharedPtr<String> m_message {};
    SharedPtr<Node> m_value {};
    bool m_safe { false };
};
}
