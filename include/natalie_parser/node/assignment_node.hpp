#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class AssignmentNode : public Node {
public:
    AssignmentNode(const Token &token, SharedPtr<Node> identifier, SharedPtr<Node> value)
        : Node { token }
        , m_identifier { identifier }
        , m_value { value } {
        assert(m_identifier);
        assert(m_value);
    }

    virtual Type type() const override { return Type::Assignment; }

    const SharedPtr<Node> identifier() const { return m_identifier; }
    const SharedPtr<Node> value() const { return m_value; }

    virtual void transform(Creator *creator) const override;

protected:
    SharedPtr<Node> m_identifier {};
    SharedPtr<Node> m_value {};
};
}
