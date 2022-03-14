#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class SuperNode : public NodeWithArgs {
public:
    SuperNode(const Token &token)
        : NodeWithArgs { token } { }

    virtual Type type() const override { return Type::Super; }

    bool parens() const { return m_parens; }
    void set_parens(bool parens) { m_parens = parens; }

    bool empty_parens() const { return m_parens && m_args.is_empty(); }

    virtual void transform(Creator *creator) const override {
        if (empty_parens()) {
            creator->set_type("super");
            return;
        } else if (args().is_empty()) {
            creator->set_type("zsuper");
            return;
        }
        creator->set_type("super");
        for (auto arg : args()) {
            creator->append(arg);
        }
    }

protected:
    bool m_parens { false };
};
}
