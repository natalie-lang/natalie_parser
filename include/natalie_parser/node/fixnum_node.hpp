#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class FixnumNode : public Node {
public:
    FixnumNode(const Token &token, long long number)
        : Node { token }
        , m_number { number } { }

    virtual Type type() const override { return Type::Fixnum; }

    virtual bool is_numeric() const override { return true; }

    long long number() const { return m_number; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("lit");
        creator->append_integer(m_number);
    }

    void negate() {
        m_number *= -1;
    }

protected:
    long long m_number;
};
}
