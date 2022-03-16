#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class ArgNode : public Node {
public:
    ArgNode(const Token &token)
        : Node { token } { }

    ArgNode(const Token &token, SharedPtr<String> name)
        : Node { token }
        , m_name { name } { }

    ArgNode(const ArgNode &other)
        : Node { other.m_token }
        , m_name { other.m_name }
        , m_block_arg { other.m_block_arg }
        , m_splat { other.m_splat }
        , m_kwsplat { other.m_kwsplat }
        , m_value { other.value() ? other.value().clone() : nullptr } { }

    virtual Node *clone() const override {
        return new ArgNode(*this);
    }

    virtual Type type() const override { return Type::Arg; }

    const SharedPtr<String> name() const { return m_name; }

    void append_name(Creator *creator) const {
        String n;
        if (m_name)
            n = m_name->clone();
        if (m_splat) {
            n.prepend_char('*');
        } else if (m_kwsplat) {
            n.prepend_char('*');
            n.prepend_char('*');
        } else if (m_block_arg) {
            n.prepend_char('&');
        }
        creator->append_symbol(n);
    }

    bool splat() const { return m_splat; }
    void set_splat(bool splat) { m_splat = splat; }

    bool kwsplat() const { return m_kwsplat; }
    void set_kwsplat(bool kwsplat) { m_kwsplat = kwsplat; }

    bool block_arg() const { return m_block_arg; }
    void set_block_arg(bool block_arg) { m_block_arg = block_arg; }

    const Node &value() const {
        if (m_value)
            return m_value.ref();
        return Node::invalid();
    }

    void set_value(Node *value) { m_value = value; }

    void add_to_locals(TM::Hashmap<const char *> &locals) {
        locals.set(m_name->c_str());
    }

    virtual void transform(Creator *creator) const override {
        creator->set_type("lasgn");
        append_name(creator);
        if (m_value)
            creator->append(m_value.ref());
    }

protected:
    SharedPtr<String> m_name {};
    bool m_block_arg { false };
    bool m_splat { false };
    bool m_kwsplat { false };
    OwnedPtr<Node> m_value {};
};
}
