#include "extconf.h"
#include "ruby.h"
#include "ruby/encoding.h"
#include "ruby/intern.h"
#include "stdio.h"

// this includes MUST come after
#include "mri_creator.hpp"
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

VALUE node_to_ruby(const NatalieParser::Node &node) {
    NatalieParser::MRICreator creator { node };
    node.transform(&creator);
    return creator.sexp();
}

VALUE parse_on_instance(VALUE self) {
    VALUE code = rb_ivar_get(self, rb_intern("@code"));
    VALUE path = rb_ivar_get(self, rb_intern("@path"));
    auto code_string = new TM::String { StringValueCStr(code) };
    auto path_string = new TM::String { StringValueCStr(path) };
    auto parser = NatalieParser::Parser { code_string, path_string };
    try {
        auto tree = parser.tree();
        VALUE ast = node_to_ruby(*tree);
        return ast;
    } catch (NatalieParser::Parser::SyntaxError &error) {
        rb_raise(rb_eSyntaxError, "%s", error.message());
    }
}

VALUE parse(int argc, VALUE *argv, VALUE self) {
    VALUE parser = rb_class_new_instance(argc, argv, Parser);
    return parse_on_instance(parser);
}

VALUE token_to_ruby(NatalieParser::Token token, bool include_location_info) {
    if (token.is_eof())
        return Qnil;
    try {
        token.validate();
    } catch (NatalieParser::Parser::SyntaxError &error) {
        rb_raise(rb_eSyntaxError, "%s", error.message());
    }
    const char *type = token.type_value();
    if (!type) abort(); // FIXME: assert no workie?
    auto hash = rb_hash_new();
    rb_hash_aset(hash, ID2SYM(rb_intern("type")), ID2SYM(rb_intern(type)));
    auto lit = token.literal_or_blank();
    switch (token.type()) {
    case NatalieParser::Token::Type::Bignum:
    case NatalieParser::Token::Type::Doc:
    case NatalieParser::Token::Type::String: {
        auto literal = token.literal_string();
        rb_hash_aset(hash, ID2SYM(rb_intern("literal")), rb_utf8_str_new(literal->c_str(), literal->length()));
        break;
    }
    case NatalieParser::Token::Type::BackRef:
    case NatalieParser::Token::Type::BareName:
    case NatalieParser::Token::Type::ClassVariable:
    case NatalieParser::Token::Type::Constant:
    case NatalieParser::Token::Type::GlobalVariable:
    case NatalieParser::Token::Type::InstanceVariable:
    case NatalieParser::Token::Type::OperatorName:
    case NatalieParser::Token::Type::Symbol:
    case NatalieParser::Token::Type::SymbolKey: {
        auto literal = token.literal_string();
        rb_hash_aset(hash, ID2SYM(rb_intern("literal")), ID2SYM(rb_intern3(literal->c_str(), literal->size(), rb_utf8_encoding())));
        break;
    }
    case NatalieParser::Token::Type::Fixnum:
    case NatalieParser::Token::Type::NthRef:
        rb_hash_aset(hash, ID2SYM(rb_intern("literal")), rb_int_new(token.get_fixnum()));
        break;
    case NatalieParser::Token::Type::Float:
        rb_hash_aset(hash, ID2SYM(rb_intern("literal")), rb_float_new(token.get_double()));
        break;
    case NatalieParser::Token::Type::InterpolatedRegexpEnd:
        if (token.has_literal()) {
            auto options = token.literal_string();
            rb_hash_aset(hash, ID2SYM(rb_intern("options")), rb_str_new(options->c_str(), options->length()));
        }
        break;
    default:
        void();
    }
    if (include_location_info) {
        rb_hash_aset(hash, ID2SYM(rb_intern("line")), rb_int_new(token.line()));
        rb_hash_aset(hash, ID2SYM(rb_intern("column")), rb_int_new(token.column()));
    }
    return hash;
}

VALUE tokens_on_instance(VALUE self, VALUE include_location_info = Qfalse) {
    VALUE code = rb_ivar_get(self, rb_intern("@code"));
    VALUE path = rb_ivar_get(self, rb_intern("@path"));
    auto code_string = new TM::String { StringValueCStr(code) };
    auto path_string = new TM::String { StringValueCStr(path) };
    auto lexer = NatalieParser::Lexer { code_string, path_string };
    auto array = rb_ary_new();
    auto the_tokens = lexer.tokens();
    for (auto token : *the_tokens) {
        auto token_value = token_to_ruby(token, RTEST(include_location_info));
        if (token_value != Qnil && token_value != Qfalse)
            rb_ary_push(array, token_value);
    }
    return array;
}

VALUE tokens(int argc, VALUE *argv, VALUE self) {
    VALUE parser = rb_class_new_instance(1, argv, Parser);
    VALUE include_location_info = argc > 1 ? argv[1] : Qfalse;
    return tokens_on_instance(parser, include_location_info);
}

void Init_natalie_parser() {
    int error;
    Sexp = rb_const_get(rb_cObject, rb_intern("Sexp"));
    Parser = rb_define_class("NatalieParser", rb_cObject);
    rb_define_method(Parser, "initialize", initialize, -1);
    rb_define_method(Parser, "parse", parse_on_instance, 0);
    rb_define_method(Parser, "tokens", tokens_on_instance, 1);
    rb_define_singleton_method(Parser, "parse", parse, -1);
    rb_define_singleton_method(Parser, "tokens", tokens, -1);
}
}
