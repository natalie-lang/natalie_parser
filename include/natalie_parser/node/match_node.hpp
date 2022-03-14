#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "natalie_parser/node/regexp_node.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class MatchNode : public Node {
public:
    MatchNode(const Token &token, RegexpNode *regexp, Node *arg, bool regexp_on_left)
        : Node { token }
        , m_regexp { regexp }
        , m_arg { arg }
        , m_regexp_on_left { regexp_on_left } { }

    ~MatchNode();

    virtual Type type() const override { return Type::Match; }

    RegexpNode *regexp() const { return m_regexp; }
    Node *arg() const { return m_arg; }
    bool regexp_on_left() const { return m_regexp_on_left; }

    virtual void transform(Creator *creator) const override;

protected:
    RegexpNode *m_regexp { nullptr };
    Node *m_arg { nullptr };
    bool m_regexp_on_left { false };
};
}
