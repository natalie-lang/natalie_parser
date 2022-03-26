#pragma once

#include "natalie_parser/node/fixnum_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class RangeNode : public Node {
public:
    RangeNode(const Token &token, Node *first, Node *last, bool exclude_end)
        : Node { token }
        , m_first { first }
        , m_last { last }
        , m_exclude_end { exclude_end } {
        assert(m_first);
        assert(m_last);
    }

    virtual Type type() const override { return Type::Range; }

    const Node &first() const { return m_first.ref(); }
    const Node &last() const { return m_last.ref(); }
    bool exclude_end() const { return m_exclude_end; }

    virtual void transform(Creator *creator) const override {
        if (m_first->type() == Node::Type::Fixnum && m_last->type() == Node::Type::Fixnum) {
            creator->set_type("lit");
            auto first_num = static_cast<const FixnumNode *>(&first())->number();
            auto last_num = static_cast<const FixnumNode *>(&last())->number();
            creator->append_range(first_num, last_num, m_exclude_end);
        } else {
            if (m_exclude_end)
                creator->set_type("dot3");
            else
                creator->set_type("dot2");
            creator->append(m_first.ref());
            creator->append(m_last.ref());
        }
    }

protected:
    OwnedPtr<Node> m_first {};
    OwnedPtr<Node> m_last {};
    bool m_exclude_end { false };
};
}
