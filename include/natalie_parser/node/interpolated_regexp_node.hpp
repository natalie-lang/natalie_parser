#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/interpolated_node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class InterpolatedRegexpNode : public InterpolatedNode {
public:
    InterpolatedRegexpNode(const Token &token)
        : InterpolatedNode { token } { }

    virtual Type type() const override { return Type::InterpolatedRegexp; }

    int options() const { return m_options; }
    void set_options(int options) { m_options = options; }

    virtual void transform(Creator *creator) const override;

private:
    int m_options { 0 };
};
}
