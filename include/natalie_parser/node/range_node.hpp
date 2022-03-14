#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/integer_node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
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

    ~RangeNode() {
        delete m_first;
        delete m_last;
    }

    virtual Type type() const override { return Type::Range; }

    Node *first() const { return m_first; }
    Node *last() const { return m_last; }
    bool exclude_end() const { return m_exclude_end; }

    virtual void transform(Creator *creator) const override {
        if (m_first->type() == Node::Type::Integer && m_last->type() == Node::Type::Integer) {
            creator->set_type("lit");
            auto first_num = static_cast<const IntegerNode *>(m_first)->number();
            auto last_num = static_cast<const IntegerNode *>(m_last)->number();
            creator->append_range(first_num, last_num, m_exclude_end);
        } else {
            if (m_exclude_end)
                creator->set_type("dot3");
            else
                creator->set_type("dot2");
            creator->append(m_first);
            creator->append(m_last);
        }
    }

protected:
    Node *m_first { nullptr };
    Node *m_last { nullptr };
    bool m_exclude_end { false };
};
}
