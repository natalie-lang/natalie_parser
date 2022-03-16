#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class DefinedNode : public Node {
public:
    DefinedNode(const Token &token, Node *arg)
        : Node { token }
        , m_arg { arg } {
        assert(arg);
    }

    virtual Type type() const override { return Type::Defined; }

    const Node &arg() const { return m_arg.ref(); }

    virtual void transform(Creator *creator) const override {
        creator->set_type("defined");
        creator->append(m_arg.ref());
    }

protected:
    OwnedPtr<Node> m_arg {};
};
}
