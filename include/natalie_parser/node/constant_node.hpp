#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class ConstantNode : public Node {
public:
    ConstantNode(const Token &token)
        : Node { token } { }

    virtual Type type() const override { return Type::Constant; }

    SharedPtr<String> name() const { return m_token.literal_string(); }

    virtual void transform(Creator *creator) const override {
        creator->set_type("const");
        creator->append_symbol(name());
    }
};
}
