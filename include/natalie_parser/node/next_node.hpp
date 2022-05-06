#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class NextNode : public Node {
public:
    NextNode(const Token &token, SharedPtr<Node> arg = {})
        : Node { token }
        , m_arg { arg } {
    }

    virtual Type type() const override { return Type::Next; }

    const Node &arg() const {
        if (m_arg)
            return m_arg.ref();
        return Node::invalid();
    }

    virtual void transform(Creator *creator) const override {
        creator->set_type("next");
        if (m_arg)
            creator->append(m_arg.ref());
    }

protected:
    SharedPtr<Node> m_arg {};
};
}
