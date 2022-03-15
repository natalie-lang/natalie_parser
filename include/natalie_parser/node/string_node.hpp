#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class StringNode : public Node {
public:
    StringNode(const Token &token, SharedPtr<String> string)
        : Node { token }
        , m_string { string } {
        assert(m_string);
    }

    StringNode(const StringNode &other)
        : StringNode { other.token(), other.string() } { }

    virtual Node *clone() const override {
        return new StringNode(*this);
    }

    virtual Type type() const override { return Type::String; }

    SharedPtr<String> string() const { return m_string; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("str");
        creator->append_string(m_string);
    }

protected:
    SharedPtr<String> m_string {};
};
}
