#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class Colon3Node : public Node {
public:
    Colon3Node(const Token &token, SharedPtr<String> name)
        : Node { token }
        , m_name { name } { }

    virtual Type type() const override { return Type::Colon3; }

    virtual bool is_assignable() const override { return true; }

    SharedPtr<String> name() const { return m_name; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("colon3");
        creator->append_symbol(m_name);
        if (creator->assignment())
            creator->wrap("cdecl");
    }

protected:
    SharedPtr<String> m_name {};
};
}
