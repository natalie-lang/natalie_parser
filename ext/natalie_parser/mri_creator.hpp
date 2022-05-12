#include "ruby.h"

#include "natalie_parser/creator.hpp"
#include "natalie_parser/node.hpp"

extern VALUE Sexp;

namespace NatalieParser {

class MRICreator : public Creator {
public:
    MRICreator(const Node &node) {
        m_file = node.file().static_cast_as<const String>();
        m_line = node.line();
        m_column = node.column();
        reset_sexp();
    }

    MRICreator(const MRICreator &other)
        : m_file { other.m_file }
        , m_line { other.m_line }
        , m_column { other.m_column } {
        reset_sexp();
    }

    virtual ~MRICreator() { }

    void reset_sexp() {
        m_sexp = rb_class_new_instance(0, nullptr, Sexp);
        rb_ivar_set(m_sexp, rb_intern("@file"), rb_str_new(m_file->c_str(), m_file->length()));
        rb_ivar_set(m_sexp, rb_intern("@line"), rb_int_new(m_line + 1));
        rb_ivar_set(m_sexp, rb_intern("@column"), rb_int_new(m_column + 1));
    }

    virtual void set_comments(const TM::String &comments) override {
        auto string_obj = rb_utf8_str_new(comments.c_str(), comments.length());
        rb_ivar_set(m_sexp, rb_intern("@comments"), string_obj);
    }

    virtual void set_type(const char *type) override {
        rb_ary_store(m_sexp, 0, ID2SYM(rb_intern(type)));
    }

    virtual void append(const Node &node) override {
        if (node.type() == Node::Type::Nil) {
            rb_ary_push(m_sexp, Qnil);
            return;
        }
        MRICreator creator { node };
        creator.set_assignment(assignment());
        node.transform(&creator);
        rb_ary_push(m_sexp, creator.sexp());
    }

    virtual void append_array(const ArrayNode &array) override {
        MRICreator creator { array };
        creator.set_assignment(assignment());
        array.ArrayNode::transform(&creator);
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

    virtual void append_integer(TM::String &number) override {
        auto string_obj = rb_utf8_str_new(number.c_str(), number.length());
        rb_ary_push(m_sexp, rb_Integer(string_obj));
    }

    virtual void append_nil() override {
        rb_ary_push(m_sexp, Qnil);
    }

    virtual void append_range(long long first, long long last, bool exclude_end) override {
        rb_ary_push(m_sexp, rb_range_new(rb_int_new(first), rb_int_new(last), exclude_end ? Qtrue : Qfalse));
    }

    virtual void append_regexp(TM::String &pattern, int options) override {
        auto regexp = rb_reg_new(pattern.c_str(), pattern.size(), options);
        rb_ary_push(m_sexp, regexp);
    }

    virtual void append_sexp(std::function<void(Creator *)> fn) override {
        MRICreator creator { *this };
        fn(&creator);
        rb_ary_push(m_sexp, creator.sexp());
    }

    virtual void append_string(TM::String &string) override {
        rb_ary_push(m_sexp, rb_utf8_str_new(string.c_str(), string.length()));
    }

    virtual void append_symbol(TM::String &name) override {
        auto str = rb_str_new(name.c_str(), name.length());
        rb_ary_push(m_sexp, rb_to_symbol(str));
    }

    virtual void append_true() override {
        rb_ary_push(m_sexp, Qtrue);
    }

    virtual void wrap(const char *type) override {
        auto inner = m_sexp;
        reset_sexp();
        set_type(type);
        rb_ary_push(m_sexp, inner);
    }

    VALUE sexp() const { return m_sexp; }

private:
    VALUE m_sexp { Qnil };
    SharedPtr<const String> m_file {};
    size_t m_line { 0 };
    size_t m_column { 0 };
};
}
