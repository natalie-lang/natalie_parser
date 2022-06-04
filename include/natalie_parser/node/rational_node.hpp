#pragma once

#include "natalie_parser/node/bignum_node.hpp"
#include "natalie_parser/node/fixnum_node.hpp"
#include "natalie_parser/node/float_node.hpp"
#include "natalie_parser/node/node.hpp"

namespace NatalieParser {

using namespace TM;

class RationalNode : public Node {
public:
    RationalNode(const Token &token, SharedPtr<Node> value)
        : Node { token }
        , m_value { value } { }

    virtual Type type() const override { return Type::Rational; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("lit");
        transform_number(creator);
        creator->make_rational_number();
    }

    void transform_number(Creator *creator) const {
        switch (m_value->type()) {
        case Node::Type::Bignum:
            creator->append_bignum(m_value.static_cast_as<BignumNode>()->number().ref());
            break;
        case Node::Type::Fixnum:
            creator->append_fixnum(m_value.static_cast_as<FixnumNode>()->number());
            break;
        case Node::Type::Float:
            creator->append_float(m_value.static_cast_as<FloatNode>()->number());
            break;
        default:
            TM_UNREACHABLE();
        }
    }

protected:
    SharedPtr<Node> m_value;
};
}
