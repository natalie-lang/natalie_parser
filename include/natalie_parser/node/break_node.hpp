#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class BreakNode : public NodeWithArgs {
public:
    BreakNode(const Token &token, Node *arg = nullptr)
        : NodeWithArgs { token }
        , m_arg { arg } { }

    ~BreakNode() {
        delete m_arg;
    }

    virtual Type type() const override { return Type::Break; }

    Node *arg() const { return m_arg; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("break");
        if (m_arg)
            creator->append(m_arg);
    }

protected:
    Node *m_arg { nullptr };
};
}
