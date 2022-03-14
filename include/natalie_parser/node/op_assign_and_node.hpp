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

class OpAssignAndNode : public OpAssignNode {
public:
    OpAssignAndNode(const Token &token, IdentifierNode *name, Node *value)
        : OpAssignNode { token, name, value } { }

    virtual Type type() const override { return Type::OpAssignAnd; }

    virtual void transform(Creator *creator) const override {
        // s(:op_asgn_and, s(:lvar, :x), s(:lasgn, :x, s(:lit, 1)))
        creator->set_type("op_asgn_and");
        creator->append(m_name);
        auto assignment = AssignmentNode { token(), m_name, m_value };
        creator->append(&assignment);
    }
};
}
