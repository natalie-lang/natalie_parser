#pragma once

#include "natalie_parser/node/array_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "natalie_parser/node/splat_node.hpp"
#include "natalie_parser/node/symbol_node.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class KeywordRestPatternNode : public Node {
public:
    KeywordRestPatternNode(const Token &token)
        : Node { token } { }

    KeywordRestPatternNode(const Token &token, String name)
        : Node { token }
        , m_name { new String(name) } { }

    KeywordRestPatternNode(const Token &token, SharedPtr<String> name)
        : Node { token }
        , m_name { name } { }

    virtual Type type() const override { return Type::KeywordRestPattern; }

    const SharedPtr<String> name() const { return m_name; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("kwrest");
        auto name = String("**");
        if (m_name)
            name.append(m_name.ref());
        creator->append_symbol(name);
    }

private:
    SharedPtr<String> m_name;
};
}
