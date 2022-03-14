#pragma once

#include "natalie_parser/node/assignment_node.hpp"
#include "natalie_parser/node/identifier_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "natalie_parser/node/op_assign_node.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class OpAssignOrNode : public OpAssignNode {
public:
    OpAssignOrNode(const Token &token, IdentifierNode *name, Node *value)
        : OpAssignNode { token, name, value } { }

    virtual Type type() const override { return Type::OpAssignOr; }

    virtual void transform(Creator *creator) const override {
        // s(:op_asgn_or, s(:lvar, :x), s(:lasgn, :x, s(:lit, 1)))
        creator->set_type("op_asgn_or");
        creator->append(m_name);
        auto assignment = AssignmentNode { token(), m_name, m_value };
        creator->append(&assignment);
    }
};
}
