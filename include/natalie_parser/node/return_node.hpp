#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class ReturnNode : public Node {
public:
    ReturnNode(const Token &token, SharedPtr<Node> value)
        : Node { token }
        , m_value { value } {
        assert(m_value);
    }

    virtual Type type() const override { return Type::Return; }

    const SharedPtr<Node> value() const { return m_value; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("return");
        if (m_value->type() != Node::Type::Nil)
            creator->append(m_value.ref());
    }

protected:
    SharedPtr<Node> m_value {};
};
}
