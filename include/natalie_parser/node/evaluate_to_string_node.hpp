#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class EvaluateToStringNode : public Node {
public:
    EvaluateToStringNode(const Token &token, Node *node)
        : Node { token }
        , m_node { node } { }

    ~EvaluateToStringNode() {
        delete m_node;
    }

    virtual Type type() const override { return Type::EvaluateToString; }

    Node *node() const { return m_node; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("evstr");
        creator->append(m_node);
    }

protected:
    Node *m_node { nullptr };
};
}
