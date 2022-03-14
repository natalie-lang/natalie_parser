#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class NextNode : public Node {
public:
    NextNode(const Token &token, Node *arg = nullptr)
        : Node { token }
        , m_arg { arg } { }

    ~NextNode() {
        delete m_arg;
    }

    virtual Type type() const override { return Type::Next; }

    Node *arg() const { return m_arg; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("next");
        if (m_arg)
            creator->append(m_arg);
    }

protected:
    Node *m_arg { nullptr };
};
}
