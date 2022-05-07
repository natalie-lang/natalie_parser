#pragma once

#include "natalie_parser/node/call_node.hpp"
#include "natalie_parser/node/identifier_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class OpAssignNode : public Node {
public:
    OpAssignNode(const Token &token, SharedPtr<Node> name, SharedPtr<Node> value)
        : Node { token }
        , m_name { name }
        , m_value { value } {
        assert(m_name);
        assert(m_value);
    }

    OpAssignNode(const Token &token, SharedPtr<String> op, SharedPtr<Node> name, SharedPtr<Node> value)
        : Node { token }
        , m_op { op }
        , m_name { name }
        , m_value { value } {
        assert(m_op);
        assert(m_name);
        assert(m_value);
    }

    virtual Type type() const override { return Type::OpAssign; }

    const SharedPtr<String> op() const { return m_op; }
    const SharedPtr<Node> name() const { return m_name; }
    const SharedPtr<Node> value() const { return m_value; }

    virtual void transform(Creator *creator) const override;

protected:
    SharedPtr<String> m_op {};
    SharedPtr<Node> m_name {};
    SharedPtr<Node> m_value {};
};
}
