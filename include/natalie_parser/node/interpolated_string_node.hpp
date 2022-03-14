#pragma once

#include "natalie_parser/node/interpolated_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class InterpolatedStringNode : public InterpolatedNode {
public:
    InterpolatedStringNode(const Token &token)
        : InterpolatedNode { token } { }

    virtual Type type() const override { return Type::InterpolatedString; }

    virtual void transform(Creator *creator) const override;
};
}
