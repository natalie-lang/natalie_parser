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
    RangeNode(const Token &token, SharedPtr<Node> first, SharedPtr<Node> last, bool exclude_end)
        : Node { token }
        , m_first { first }
        , m_last { last }
        , m_exclude_end { exclude_end } {
        assert(m_first);
        assert(m_last);
    }

    virtual Type type() const override { return Type::Range; }

    const SharedPtr<Node> first() const { return m_first; }
    const SharedPtr<Node> last() const { return m_last; }
    bool exclude_end() const { return m_exclude_end; }

    virtual void transform(Creator *creator) const override {
        if (m_first->type() == Node::Type::Fixnum && m_last->type() == Node::Type::Fixnum) {
            creator->set_type("lit");
            auto first_num = m_first.static_cast_as<FixnumNode>()->number();
            auto last_num = m_last.static_cast_as<FixnumNode>()->number();
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
    SharedPtr<Node> m_first {};
    SharedPtr<Node> m_last {};
    bool m_exclude_end { false };
};
}
