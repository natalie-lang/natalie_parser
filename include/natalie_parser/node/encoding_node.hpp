#pragma once

#include "natalie_parser/node/node.hpp"

namespace NatalieParser {

using namespace TM;

class EncodingNode : public Node {
public:
    EncodingNode(const Token &token)
        : Node { token } { }

    virtual Type type() const override { return Type::Encoding; }

    virtual void transform(Creator *creator) const override {
        // s(:colon2, s(:const, :Encoding), :UTF_8)
        creator->set_type("colon2");
        creator->append_sexp([&](Creator *c) {
            c->set_type("const");
            c->append_symbol("Encoding");
        });
        creator->append_symbol("UTF_8"); // TODO: support file-encodings other than UTF-8
    }
};
}
