#pragma once

#include "natalie_parser/node/array_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class ArrayPatternNode : public ArrayNode {
public:
    ArrayPatternNode(const Token &token)
        : ArrayNode { token } { }

    virtual Type type() const override { return Type::ArrayPattern; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("array_pat");
        if (!m_nodes.is_empty())
            creator->append_nil(); // NOTE: I don't know what this nil is for
        for (auto node : m_nodes)
            creator->append(node);
    }
};
}
