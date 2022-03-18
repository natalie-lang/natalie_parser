#pragma once

#include "natalie_parser/creator.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class NodeWithArgs : public Node {
public:
    NodeWithArgs(const Token &token)
        : Node { token } { }

    NodeWithArgs(const Token &token, const Vector<Node *> &args)
        : Node { token } {
        for (auto arg : args)
            add_arg(arg);
    }

    NodeWithArgs(const NodeWithArgs &other)
        : NodeWithArgs { other.token() } {
        for (auto arg : other.args())
            add_arg(arg->clone());
    }

    ~NodeWithArgs() {
        for (auto arg : m_args)
            delete arg;
    }

    void add_arg(Node *arg) {
        m_args.push(arg);
    }

    Vector<Node *> &args() { return m_args; }
    const Vector<Node *> &args() const { return m_args; }

    void append_method_or_block_args(Creator *creator) const;

protected:
    Vector<Node *> m_args {};
};
}
