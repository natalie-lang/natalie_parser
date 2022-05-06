#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class PinNode : public Node {
public:
    PinNode(const Token &token, SharedPtr<Node> identifier)
        : Node { token }
        , m_identifier { identifier } {
        assert(m_identifier);
    }

    virtual Type type() const override { return Type::Pin; }

    const SharedPtr<Node> identifier() const { return m_identifier; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("pin");
        creator->append(m_identifier.ref());
    }

protected:
    SharedPtr<Node> m_identifier {};
};
}
