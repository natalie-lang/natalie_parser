#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class RegexpNode : public Node {
public:
    RegexpNode(const Token &token, SharedPtr<String> pattern)
        : Node { token }
        , m_pattern { pattern } {
        assert(m_pattern);
    }

    virtual Type type() const override { return Type::Regexp; }

    SharedPtr<String> pattern() const { return m_pattern; }

    int options() const { return m_options; }
    void set_options(int options) { m_options = options; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("lit");
        creator->append_regexp(m_pattern, m_options);
    }

protected:
    SharedPtr<String> m_pattern {};
    int m_options { 0 };
};
}
