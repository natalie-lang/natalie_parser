#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class NotNode : public Node {
public:
    NotNode(const Token &token, SharedPtr<Node> expression)
        : Node { token }
        , m_expression { expression } {
        assert(m_expression);
    }

    virtual Type type() const override { return Type::Not; }

    const SharedPtr<Node> expression() const { return m_expression; }

    void set_expression(SharedPtr<Node> expression) { m_expression = expression; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("call");
        creator->append(m_expression.ref());
        auto message = String("!");
        creator->append_symbol(message);
    }

protected:
    SharedPtr<Node> m_expression {};
};
}
