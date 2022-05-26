#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class ShadowArgNode : public Node {
public:
    ShadowArgNode(const Token &token, SharedPtr<String> name)
        : Node { token }
        , m_name { name } { }

    virtual Type type() const override { return Type::ShadowArg; }

    const SharedPtr<String> name() const { return m_name; }

    void add_to_locals(TM::Hashmap<TM::String> &locals) {
        locals.set(m_name->c_str());
    }

    virtual void transform(Creator *creator) const override {
        creator->set_type("shadow");
        creator->append_symbol(m_name);
    }

protected:
    SharedPtr<String> m_name {};
};
}
