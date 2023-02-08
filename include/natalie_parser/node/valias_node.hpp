#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class ValiasNode : public Node {
public:
    ValiasNode(const Token &token, SharedPtr<String> new_name, SharedPtr<String> existing_name)
        : Node { token }
        , m_new_name { new_name }
        , m_existing_name { existing_name } {
        assert(m_new_name);
        assert(m_existing_name);
    }

    virtual Type type() const override { return Type::Valias; }

    SharedPtr<String> new_name() const { return m_new_name; }
    SharedPtr<String> existing_name() const { return m_existing_name; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("valias");
        creator->append_symbol(m_new_name);
        creator->append_symbol(m_existing_name);
    }

protected:
    SharedPtr<String> m_new_name {};
    SharedPtr<String> m_existing_name {};
};
}
