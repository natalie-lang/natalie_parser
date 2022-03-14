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

VALUE node_to_ruby(NatalieParser::Node *node) {
    NatalieParser::MRICreator creator;
    node->transform(&creator);
    return creator.sexp();
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
        return node_to_ruby(tree);
    } catch (NatalieParser::Parser::SyntaxError &error) {
        rb_raise(rb_eSyntaxError, "%s", error.message());
    }
}

VALUE parse(int argc, VALUE *argv, VALUE self) {
    VALUE parser = rb_class_new_instance(argc, argv, Parser);
    return parse_on_instance(parser);
}

VALUE token_to_ruby(NatalieParser::Token token) {
    if (token.is_eof())
        return Qnil;
    try {
        token.validate();
    } catch (NatalieParser::Parser::SyntaxError &error) {
        rb_raise(rb_eSyntaxError, "%s", error.message());
    }
    const char *type = token.type_value();
    auto hash = rb_hash_new();
    rb_hash_aset(hash, ID2SYM(rb_intern("type")), ID2SYM(rb_intern(type)));
    auto lit = token.literal_or_blank();
    switch (token.type()) {
    case NatalieParser::Token::Type::PercentLowerI:
    case NatalieParser::Token::Type::PercentUpperI:
    case NatalieParser::Token::Type::PercentLowerW:
    case NatalieParser::Token::Type::PercentUpperW:
    case NatalieParser::Token::Type::Regexp:
    case NatalieParser::Token::Type::String:
        rb_hash_aset(hash, ID2SYM(rb_intern("literal")), rb_str_new2(lit));
        break;
    case NatalieParser::Token::Type::BareName:
    case NatalieParser::Token::Type::ClassVariable:
    case NatalieParser::Token::Type::Constant:
    case NatalieParser::Token::Type::GlobalVariable:
    case NatalieParser::Token::Type::InstanceVariable:
    case NatalieParser::Token::Type::Symbol:
    case NatalieParser::Token::Type::SymbolKey:
        rb_hash_aset(hash, ID2SYM(rb_intern("literal")), ID2SYM(rb_intern(lit)));
        break;
    case NatalieParser::Token::Type::Float:
        rb_hash_aset(hash, ID2SYM(rb_intern("literal")), rb_float_new(token.get_double()));
        break;
    case NatalieParser::Token::Type::Integer:
        rb_hash_aset(hash, ID2SYM(rb_intern("literal")), rb_int_new(token.get_integer()));
        break;
    case NatalieParser::Token::Type::InterpolatedRegexpEnd:
        if (token.options()) {
            auto options = token.options().value();
            rb_hash_aset(hash, ID2SYM(rb_intern("options")), rb_str_new(options->c_str(), options->length()));
        }
        break;
    default:
        void();
    }
    return hash;
}

VALUE tokens_on_instance(VALUE self) {
    VALUE code = rb_ivar_get(self, rb_intern("@code"));
    VALUE path = rb_ivar_get(self, rb_intern("@path"));
    auto code_string = new TM::String { StringValueCStr(code) };
    auto path_string = new TM::String { StringValueCStr(path) };
    auto lexer = NatalieParser::Lexer { code_string, path_string };
    auto array = rb_ary_new();
    auto the_tokens = lexer.tokens();
    for (auto token : *the_tokens) {
        auto token_value = token_to_ruby(token);
        if (token_value != Qnil && token_value != Qfalse)
            rb_ary_push(array, token_value);
    }
    return array;
}

VALUE tokens(int argc, VALUE *argv, VALUE self) {
    VALUE parser = rb_class_new_instance(argc, argv, Parser);
    return tokens_on_instance(parser);
}

void Init_natalie_parser() {
    int error;
    Sexp = rb_const_get(rb_cObject, rb_intern("Sexp"));
    Parser = rb_define_class("Parser", rb_cObject);
    rb_define_method(Parser, "initialize", initialize, -1);
    rb_define_method(Parser, "parse", parse_on_instance, 0);
    rb_define_method(Parser, "tokens", tokens_on_instance, 0);
    rb_define_singleton_method(Parser, "parse", parse, -1);
    rb_define_singleton_method(Parser, "tokens", tokens, -1);
}
}
