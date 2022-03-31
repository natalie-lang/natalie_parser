#pragma once

#include "natalie_parser/node/node.hpp"

namespace NatalieParser {

using namespace TM;

class BackRefNode : public Node {
public:
    BackRefNode(const Token &token, char type)
        : Node { token }
        , m_type { type } { }

    virtual Type type() const override { return Type::BackRef; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("back_ref");
        auto type = String(m_type);
        creator->append_symbol(type);
    }

protected:
    char m_type { 0 };
};
}
