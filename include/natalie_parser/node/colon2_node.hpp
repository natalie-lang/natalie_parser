#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class Colon2Node : public Node {
public:
    Colon2Node(const Token &token, Node *left, SharedPtr<String> name)
        : Node { token }
        , m_left { left }
        , m_name { name } {
        assert(m_left);
        assert(m_name);
    }

    Colon2Node(const Colon2Node &other)
        : Colon2Node { other.token(), other.left().clone(), other.name() } { }

    virtual Node *clone() const override {
        return new Colon2Node(*this);
    }

    virtual Type type() const override { return Type::Colon2; }

    const Node &left() const { return m_left.ref(); }
    SharedPtr<String> name() const { return m_name; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("colon2");
        creator->with_assignment(false, [&]() {
            creator->append(m_left.ref());
        });
        creator->append_symbol(m_name);
        if (creator->assignment())
            creator->wrap("cdecl");
    }

protected:
    OwnedPtr<Node> m_left {};
    SharedPtr<String> m_name {};
};
}
