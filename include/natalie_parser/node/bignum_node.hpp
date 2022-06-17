#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class BignumNode : public Node {
public:
    BignumNode(const Token &token, SharedPtr<String> number)
        : Node { token }
        , m_number { number } { }

    virtual Type type() const override { return Type::Bignum; }

    virtual bool is_numeric() const override { return true; }

    SharedPtr<String> number() const { return m_number; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("lit");
        creator->append_bignum(*m_number);
    }

    void negate() {
        assert(m_number->at(0) != '-');
        m_number->prepend_char('-');
    }

    bool negative() const {
        return m_number->at(0) == '-';
    }

protected:
    SharedPtr<String> m_number;
};
}
