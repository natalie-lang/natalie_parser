#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class SplatValueNode : public Node {
public:
    SplatValueNode(const Token &token, SharedPtr<Node> value)
        : Node { token }
        , m_value { value } { }

    virtual Type type() const override { return Type::SplatValue; }

    const SharedPtr<Node> value() const { return m_value; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("svalue");
        creator->append(m_value.ref());
    }

protected:
    SharedPtr<Node> m_value {};
};

}
