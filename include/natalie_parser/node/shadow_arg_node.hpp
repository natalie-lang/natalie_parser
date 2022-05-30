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
    ShadowArgNode(const Token &token)
        : Node { token } { }

    virtual Type type() const override { return Type::ShadowArg; }

    const Vector<SharedPtr<String>> &names() const { return m_names; }

    void add_name(SharedPtr<String> name) {
        m_names.push(name);
    }

    void add_to_locals(TM::Hashmap<TM::String> &locals) {
        for (auto name : m_names)
            locals.set(name->c_str());
    }

    virtual void transform(Creator *creator) const override {
        creator->set_type("shadow");
        for (auto name : m_names)
            creator->append_symbol(name);
    }

protected:
    Vector<SharedPtr<String>> m_names {};
};
}
