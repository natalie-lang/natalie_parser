#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class StabbyProcNode : public NodeWithArgs {
public:
    StabbyProcNode(const Token &token, bool has_args, const Vector<SharedPtr<Node>> &args)
        : NodeWithArgs { token, args }
        , m_has_args { has_args } { }

    virtual Type type() const override { return Type::StabbyProc; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("lambda");
    }

    bool has_args() const { return m_has_args; }

private:
    bool m_has_args { false };
};
}
