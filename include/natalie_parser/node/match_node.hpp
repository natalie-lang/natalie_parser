#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "natalie_parser/node/regexp_node.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class MatchNode : public Node {
public:
    MatchNode(const Token &token, RegexpNode *regexp, Node *arg, bool regexp_on_left)
        : Node { token }
        , m_regexp { regexp }
        , m_arg { arg }
        , m_regexp_on_left { regexp_on_left } {
        assert(m_regexp);
        assert(m_arg);
    }

    virtual Type type() const override { return Type::Match; }

    const RegexpNode &regexp() const { return m_regexp.ref(); }
    const Node &arg() const { return m_arg.ref(); }
    bool regexp_on_left() const { return m_regexp_on_left; }

    virtual void transform(Creator *creator) const override;

protected:
    OwnedPtr<RegexpNode> m_regexp {};
    OwnedPtr<Node> m_arg {};
    bool m_regexp_on_left { false };
};
}
