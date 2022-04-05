#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class CallNode : public NodeWithArgs {
public:
    CallNode(const Token &token, Node *receiver, SharedPtr<String> message)
        : NodeWithArgs { token }
        , m_receiver { receiver }
        , m_message { message } {
        assert(m_receiver);
        assert(m_message);
    }

    CallNode(const Token &token, CallNode &node)
        : NodeWithArgs { token }
        , m_receiver { node.receiver().clone() }
        , m_message { node.m_message } {
        for (auto arg : node.m_args) {
            add_arg(arg->clone());
        }
    }

    CallNode(const CallNode &other)
        : NodeWithArgs { other.token() }
        , m_receiver { other.receiver().clone() }
        , m_message { other.message() } {
        for (auto arg : other.args()) {
            add_arg(arg->clone());
        }
    }

    virtual Node *clone() const override {
        return new CallNode(*this);
    }

    virtual Type type() const override { return Type::Call; }

    virtual bool is_callable() const override { return true; }
    virtual bool can_accept_a_block() const override { return true; }

    const Node &receiver() const { return m_receiver.ref(); }
    void set_receiver(Node *receiver) { m_receiver = receiver; }

    SharedPtr<String> message() const { return m_message; }

    void set_message(SharedPtr<String> message) {
        assert(message);
        m_message = message;
    }

    void set_message(const char *message) {
        assert(message);
        m_message = new String(message);
    }

    virtual void transform(Creator *creator) const override {
        if (creator->assignment()) {
            creator->set_type("attrasgn");
            creator->with_assignment(false, [&]() {
                creator->append(m_receiver.ref());
            });
            auto message = m_message->clone();
            message.append_char('=');
            creator->append_symbol(message);
        } else {
            creator->set_type("call");
            creator->append(m_receiver.ref());
            creator->append_symbol(m_message);
        }
        creator->with_assignment(false, [&]() {
            for (auto arg : args())
                creator->append(arg);
        });
    }

protected:
    OwnedPtr<Node> m_receiver {};
    SharedPtr<String> m_message {};
};
}
