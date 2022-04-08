#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class StabbyProcNode : public NodeWithArgs {
public:
    StabbyProcNode(const Token &token, bool has_args, const Vector<Node *> &args)
        : NodeWithArgs { token, args }
        , m_has_args { has_args } { }

    StabbyProcNode(const StabbyProcNode &other)
        : NodeWithArgs { other }
        , m_has_args { other.m_has_args } { }

    virtual Node *clone() const override {
        return new StabbyProcNode(*this);
    }
    virtual Type type() const override { return Type::StabbyProc; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("lambda");
    }

    bool has_args() const { return m_has_args; }

private:
    bool m_has_args { false };
};
}
