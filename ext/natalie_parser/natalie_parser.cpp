#include "extconf.h"
#include "ruby.h"
#include "ruby/intern.h"
#include "stdio.h"

// this includes MUST come after
#include "natalie_parser/creator.hpp"
#include "natalie_parser/parser.hpp"

VALUE Parser;
VALUE Sexp;

VALUE s(std::initializer_list<VALUE> list) {
    auto sexp = rb_class_new_instance(0, nullptr, Sexp);
    for (auto item : list)
        rb_ary_push(sexp, item);
    return sexp;
}

namespace NatalieParser {

class MRICreator : public Creator {
public:
    MRICreator() {
        reset();
    }

    void reset() {
        m_sexp = rb_class_new_instance(0, nullptr, Sexp);
    }

    virtual void set_type(const char *type) override {
        rb_ary_store(m_sexp, 0, ID2SYM(rb_intern(type)));
    }

    virtual void append(const Node *node) override {
        if (node->type() == Node::Type::Nil) {
            rb_ary_push(m_sexp, Qnil);
            return;
        }
        MRICreator creator;
        creator.set_assignment(assignment());
        node->transform(&creator);
        rb_ary_push(m_sexp, creator.sexp());
    }

    virtual void append_array(const ArrayNode *array) override {
        MRICreator creator;
        creator.set_assignment(assignment());
        array->ArrayNode::transform(&creator);
        rb_ary_push(m_sexp, creator.sexp());
    }

    virtual void append_false() override {
        rb_ary_push(m_sexp, Qfalse);
    }

    virtual void append_float(double number) override {
        rb_ary_push(m_sexp, rb_float_new(number));
    }

    virtual void append_integer(long long number) override {
        rb_ary_push(m_sexp, rb_int_new(number));
    }

    virtual void append_nil() override {
        rb_ary_push(m_sexp, Qnil);
    }

    virtual void append_range(long long first, long long last, bool exclude_end) override {
        rb_ary_push(m_sexp, rb_range_new(rb_int_new(first), rb_int_new(last), exclude_end ? Qtrue : Qfalse));
    }

    virtual void append_regexp(TM::SharedPtr<TM::String> pattern, int options) override {
        auto regexp = rb_reg_new(pattern->c_str(), pattern->size(), options);
        rb_ary_push(m_sexp, regexp);
    }

    virtual void append_sexp(std::function<void(Creator *)> fn) override {
        MRICreator creator;
        fn(&creator);
        rb_ary_push(m_sexp, creator.sexp());
    }

    virtual void append_string(TM::SharedPtr<TM::String> str) override {
        rb_ary_push(m_sexp, rb_str_new(str->c_str(), str->length()));
    }

    virtual void append_symbol(TM::SharedPtr<TM::String> name) override {
        // FIXME: check if there is a way to avoid creation of the Ruby String obj
        rb_ary_push(m_sexp, ID2SYM(rb_intern_str(rb_str_new(name->c_str(), name->length()))));
    }

    virtual void append_symbol(TM::String name) override {
        // FIXME: check if there is a way to avoid creation of the Ruby String obj
        rb_ary_push(m_sexp, ID2SYM(rb_intern_str(rb_str_new(name.c_str(), name.length()))));
    }

    virtual void append_true() override {
        rb_ary_push(m_sexp, Qtrue);
    }

    virtual void wrap(const char *type) override {
        auto inner = m_sexp;
        reset();
        set_type(type);
        rb_ary_push(m_sexp, inner);
    }

    VALUE sexp() const { return m_sexp; }

private:
    VALUE m_sexp { Qnil };
};
}

extern "C" {

VALUE to_ruby(NatalieParser::Node *node) {
    NatalieParser::MRICreator creator;
    node->transform(&creator);
    return creator.sexp();
}

VALUE initialize(int argc, VALUE *argv, VALUE self) {
    if (argc < 1 || argc > 2)
        rb_raise(rb_eSyntaxError,
            "wrong number of arguments (given %d, expected 1..2)", argc);
    rb_ivar_set(self, rb_intern("@code"), argv[0]);
    VALUE path;
    if (argc > 1)
        path = argv[1];
    else
        path = rb_str_new_cstr("(string)");
    rb_ivar_set(self, rb_intern("@path"), path);
    return self;
}

VALUE parse_on_instance(VALUE self) {
    VALUE code = rb_ivar_get(self, rb_intern("@code"));
    VALUE path = rb_ivar_get(self, rb_intern("@path"));
    auto code_string = new TM::String { StringValueCStr(code) };
    auto path_string = new TM::String { StringValueCStr(path) };
    auto parser = NatalieParser::Parser { code_string, path_string };
    NatalieParser::Node *tree;
    try {
        tree = parser.tree();
        return to_ruby(tree);
    } catch (NatalieParser::Parser::SyntaxError &error) {
        rb_raise(rb_eSyntaxError, "%s", error.message());
    }
}

VALUE parse(int argc, VALUE *argv, VALUE self) {
    VALUE parser = rb_class_new_instance(argc, argv, Parser);
    return parse_on_instance(parser);
}

VALUE s(int argc, VALUE *argv, VALUE self) {
    VALUE sexp = rb_class_new_instance(0, nullptr, Sexp);
    for (int i = 0; i < argc; ++i)
        rb_ary_push(sexp, argv[i]);
    return sexp;
}

void Init_natalie_parser() {
    int error;
    Sexp = rb_const_get(rb_cObject, rb_intern("Sexp"));
    Parser = rb_define_class("Parser", rb_cObject);
    rb_define_method(Parser, "parse", parse_on_instance, 0);
    rb_define_method(Parser, "initialize", initialize, -1);
    rb_define_singleton_method(Parser, "parse", parse, -1);
    rb_define_method(rb_cObject, "s", s, -1);
}
}
