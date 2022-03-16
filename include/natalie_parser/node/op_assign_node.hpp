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
    OpAssignNode(const Token &token, IdentifierNode *name, Node *value)
        : Node { token }
        , m_name { name }
        , m_value { value } {
        assert(m_name);
        assert(m_value);
    }

    OpAssignNode(const Token &token, SharedPtr<String> op, IdentifierNode *name, Node *value)
        : Node { token }
        , m_op { op }
        , m_name { name }
        , m_value { value } {
        assert(m_op);
        assert(m_name);
        assert(m_value);
    }

    OpAssignNode(const OpAssignNode &other)
        : OpAssignNode {
            other.token(),
            other.op(),
            new IdentifierNode { other.name() },
            other.value().clone(),
        } { }

    virtual Node *clone() const override {
        return new OpAssignNode(*this);
    }

    virtual Type type() const override { return Type::OpAssign; }

    SharedPtr<String> op() const { return m_op; }
    const IdentifierNode &name() const { return m_name.ref(); }
    const Node &value() const { return m_value.ref(); }

    virtual void transform(Creator *creator) const override {
        assert(op());
        creator->with_assignment(true, [&]() {
            m_name->transform(creator);
        });
        auto call = CallNode { token(), m_name->clone(), m_op };
        call.add_arg(m_value->clone());
        creator->append(&call);
    }

protected:
    SharedPtr<String> m_op {};
    OwnedPtr<IdentifierNode> m_name {};
    OwnedPtr<Node> m_value {};
};
}
