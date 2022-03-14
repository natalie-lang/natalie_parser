#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class ToArrayNode : public Node {
public:
    ToArrayNode(const Token &token, Node *value)
        : Node { token }
        , m_value { value } { }

    ~ToArrayNode() {
        delete m_value;
    }

    virtual Type type() const override { return Type::ToArray; }

    Node *value() const { return m_value; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("to_ary");
        creator->append(m_value);
    }

protected:
    Node *m_value { nullptr };
};
}
