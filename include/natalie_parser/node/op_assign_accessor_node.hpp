#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class OpAssignAccessorNode : public NodeWithArgs {
public:
    OpAssignAccessorNode(const Token &token, SharedPtr<String> op, Node *receiver, SharedPtr<String> message, Node *value, Vector<Node *> &args)
        : NodeWithArgs { token, args }
        , m_op { op }
        , m_receiver { receiver }
        , m_message { message }
        , m_value { value } {
        assert(m_op);
        assert(m_receiver);
        assert(m_message);
        assert(m_value);
    }

    ~OpAssignAccessorNode() {
        delete m_receiver;
        delete m_value;
    }

    virtual Type type() const override { return Type::OpAssignAccessor; }

    SharedPtr<String> op() const { return m_op; }
    Node *receiver() const { return m_receiver; }
    SharedPtr<String> message() const { return m_message; }
    Node *value() const { return m_value; }

    virtual void transform(Creator *creator) const override {
        if (*m_message == "[]=") {
            creator->set_type("op_asgn1");
            creator->append(m_receiver);
            creator->append_sexp([&](Creator *c) {
                c->set_type("arglist");
                for (auto arg : args())
                    c->append(arg);
            });
            creator->append_symbol(m_op);
            creator->append(m_value);
            return;
        }
        assert(args().is_empty());
        creator->set_type("op_asgn2");
        creator->append(m_receiver);
        creator->append_symbol(m_message);
        creator->append_symbol(m_op);
        creator->append(m_value);
    }

protected:
    SharedPtr<String> m_op {};
    Node *m_receiver { nullptr };
    SharedPtr<String> m_message {};
    Node *m_value { nullptr };
};
}
