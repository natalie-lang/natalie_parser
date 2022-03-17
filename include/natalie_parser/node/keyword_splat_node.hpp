#pragma once

#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "tm/hashmap.hpp"
#include "tm/owned_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class KeywordSplatNode : public Node {
public:
    KeywordSplatNode(const Token &token)
        : Node { token } { }

    KeywordSplatNode(const Token &token, Node *node)
        : Node { token }
        , m_node { node } {
        assert(m_node);
    }

    virtual Type type() const override { return Type::KeywordSplat; }

    const Node &node() const { return m_node.ref(); }

    virtual void transform(Creator *creator) const override {
        creator->set_type("hash");
        creator->append_sexp([&](Creator *c) {
            c->set_type("kwsplat");
            if (m_node)
                c->append(m_node.ref());
        });
    }

protected:
    OwnedPtr<Node> m_node {};
};

}