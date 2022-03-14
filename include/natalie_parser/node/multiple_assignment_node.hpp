#pragma once

#include "natalie_parser/node/array_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class MultipleAssignmentNode : public ArrayNode {
public:
    MultipleAssignmentNode(const Token &token)
        : ArrayNode { token } { }

    virtual Type type() const override { return Type::MultipleAssignment; }

    void add_locals(TM::Hashmap<const char *> &);

    virtual void transform(Creator *creator) const override {
        creator->with_assignment(true, [&]() {
            creator->set_type("masgn");
            creator->append_array(this);
        });
    }
};
}
