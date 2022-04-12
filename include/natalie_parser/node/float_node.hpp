#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class FloatNode : public Node {
public:
    FloatNode(const Token &token, double number)
        : Node { token }
        , m_number { number } { }

    FloatNode(const FloatNode &other)
        : FloatNode { other.token(), other.number() } { }

    virtual Node *clone() const override {
        return new FloatNode(*this);
    }

    virtual Type type() const override { return Type::Float; }

    virtual bool is_numeric() const override { return true; }

    double number() const { return m_number; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("lit");
        creator->append_float(m_number);
    }

    void negate() {
        m_number *= -1;
    }

protected:
    double m_number;
};
}
