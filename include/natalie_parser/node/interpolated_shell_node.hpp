#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/interpolated_node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class InterpolatedShellNode : public InterpolatedNode {
public:
    InterpolatedShellNode(const Token &token)
        : InterpolatedNode { token } { }

    virtual Type type() const override { return Type::InterpolatedShell; }

    virtual void transform(Creator *creator) const override;
};
}
