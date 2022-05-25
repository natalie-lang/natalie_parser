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

    NodeWithArgs(const Token &token, const Vector<SharedPtr<Node>> &args)
        : Node { token } {
        for (auto arg : args)
            add_arg(arg);
    }

    NodeWithArgs(const NodeWithArgs &other)
        : NodeWithArgs { other.token() } {
        for (auto arg : other.args())
            add_arg(arg);
    }

    void add_arg(SharedPtr<Node> arg) {
        // TODO: error if BlockPass already added (must be last)
        m_args.push(arg);
    }

    bool has_block_pass() const override {
        return m_args.size() > 0 && m_args.last()->type() == Node::Type::BlockPass;
    }

    Vector<SharedPtr<Node>> &args() { return m_args; }
    const Vector<SharedPtr<Node>> &args() const { return m_args; }

    void append_method_or_block_args(Creator *creator) const;

protected:
    Vector<SharedPtr<Node>> m_args {};
};
}
