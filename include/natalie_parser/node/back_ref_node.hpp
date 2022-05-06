#pragma once

#include "natalie_parser/node/node.hpp"

namespace NatalieParser {

using namespace TM;

class BackRefNode : public Node {
public:
    BackRefNode(const Token &token, char ref_type)
        : Node { token }
        , m_ref_type { ref_type } { }

    virtual Type type() const override { return Type::BackRef; }

    char ref_type() const { return m_ref_type; }

    virtual void transform(Creator *creator) const override {
        creator->set_type("back_ref");
        auto type = String(m_ref_type);
        creator->append_symbol(type);
    }

protected:
    char m_ref_type { 0 };
};
}
