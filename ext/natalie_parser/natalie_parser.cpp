#include "extconf.h"
#include "ruby.h"
#include "ruby/intern.h"
#include "stdio.h"

// this includes MUST come after
#include "natalie_parser/creator/mri.hpp"
#include "natalie_parser/parser.hpp"

VALUE Parser;
VALUE Sexp;

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

void Init_natalie_parser() {
    int error;
    Sexp = rb_const_get(rb_cObject, rb_intern("Sexp"));
    Parser = rb_define_class("Parser", rb_cObject);
    rb_define_method(Parser, "parse", parse_on_instance, 0);
    rb_define_method(Parser, "initialize", initialize, -1);
    rb_define_singleton_method(Parser, "parse", parse, -1);
}
}
