#include "ruby.h"

#include "natalie_parser/creator.hpp"
#include "natalie_parser/node.hpp"

extern VALUE Sexp;

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

    virtual void append_regexp(TM::String &pattern, int options) override {
        auto regexp = rb_reg_new(pattern.c_str(), pattern.size(), options);
        rb_ary_push(m_sexp, regexp);
    }

    virtual void append_sexp(std::function<void(Creator *)> fn) override {
        MRICreator creator;
        fn(&creator);
        rb_ary_push(m_sexp, creator.sexp());
    }

    virtual void append_string(TM::String &string) override {
        rb_ary_push(m_sexp, rb_str_new(string.c_str(), string.length()));
    }

    virtual void append_symbol(TM::String &name) override {
        rb_ary_push(m_sexp, rb_check_id_cstr(name.c_str(), name.length(), NULL));
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
