#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class IntegerNode : public Node {
public:
    IntegerNode(const Token &token, long long number)
        : Node { token }
        , m_number { number } { }

    IntegerNode(const IntegerNode &other)
        : IntegerNode { other.token(), other.number() } { }

    virtual Node *clone() const override {
        return new IntegerNode(*this);
    }

    virtual Type type() const override { return Type::Integer; }

    virtual bool is_numeric() const override { return true; }

    long long number() const { return m_number; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("lit");
        creator->append_integer(m_number);
    }

protected:
    long long m_number;
};
}
