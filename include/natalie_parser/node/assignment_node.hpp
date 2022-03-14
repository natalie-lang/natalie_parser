#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class AssignmentNode : public Node {
public:
    AssignmentNode(const Token &token, Node *identifier, Node *value)
        : Node { token }
        , m_identifier { identifier }
        , m_value { value } {
        assert(m_identifier);
        assert(m_value);
    }

    ~AssignmentNode() {
        delete m_identifier;
        delete m_value;
    }

    virtual Type type() const override { return Type::Assignment; }

    Node *identifier() const { return m_identifier; }
    Node *value() const { return m_value; }

    virtual void transform(Creator *creator) const override;

protected:
    Node *m_identifier { nullptr };
    Node *m_value { nullptr };
};
}
