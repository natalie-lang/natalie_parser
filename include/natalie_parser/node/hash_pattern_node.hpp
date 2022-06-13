#pragma once

#include "natalie_parser/node/hash_node.hpp"
#include "natalie_parser/node/node.hpp"
#include "natalie_parser/node/node_with_args.hpp"
#include "natalie_parser/node/symbol_key_node.hpp"
#include "tm/hashmap.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class HashPatternNode : public HashNode {
public:
    HashPatternNode(const Token &token)
        : HashNode { token } { }

    virtual Type type() const override { return Type::HashPattern; }

    void add_last_node_to_locals(TM::Hashmap<TM::String> &locals) {
        auto last_node = m_nodes.last();
        assert(last_node->type() == Node::Type::SymbolKey);
        locals.set(last_node.static_cast_as<SymbolKeyNode>()->name().ref());
    }

    virtual void transform(Creator *creator) const override {
        creator->set_type("hash_pat");
        creator->append_nil(); // NOTE: I don't know what this nil is for
        for (auto node : m_nodes)
            creator->append(node);
    }
};
}
