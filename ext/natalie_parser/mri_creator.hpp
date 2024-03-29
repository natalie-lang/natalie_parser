#include "ruby.h"
#include "ruby/encoding.h"
#include "ruby/intern.h"

#include "natalie_parser/creator.hpp"
#include "natalie_parser/node.hpp"

extern VALUE Sexp;

namespace NatalieParser {

class MRICreator : public Creator {
public:
    MRICreator(const Node &node)
        : Creator { node.file().static_cast_as<const String>(), node.line(), node.column() } {
        reset_sexp();
    }

    MRICreator(const MRICreator &other)
        : Creator { other.file(), other.line(), other.column() } {
        reset_sexp();
    }

    virtual ~MRICreator() { }

    virtual void reset_sexp() override {
        m_sexp = rb_class_new_instance(0, nullptr, Sexp);
        rb_ivar_set(m_sexp, rb_intern("@file"), get_file_string(*file()));
        rb_ivar_set(m_sexp, rb_intern("@line"), rb_int_new(line() + 1));
        rb_ivar_set(m_sexp, rb_intern("@column"), rb_int_new(column() + 1));
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

    virtual void append_bignum(TM::String &number) override {
        auto string_obj = rb_utf8_str_new(number.c_str(), number.length());
        auto num = rb_Integer(string_obj);
        rb_ary_push(m_sexp, num);
    }

    virtual void append_fixnum(long long number) override {
        auto num = rb_int_new(number);
        rb_ary_push(m_sexp, num);
    }

    virtual void append_float(double number) override {
        auto num = rb_float_new(number);
        rb_ary_push(m_sexp, num);
    }

    virtual void append_nil() override {
        rb_ary_push(m_sexp, Qnil);
    }

    virtual void append_range(long long first, long long last, bool exclude_end) override {
        rb_ary_push(m_sexp, rb_range_new(rb_int_new(first), rb_int_new(last), exclude_end ? Qtrue : Qfalse));
    }

    virtual void append_regexp(TM::String &pattern, int options) override {
        auto encoding = pattern.contains_utf8_encoded_multibyte_characters() ? rb_utf8_encoding() : rb_ascii8bit_encoding();
        auto regexp = rb_enc_reg_new(pattern.c_str(), pattern.size(), encoding, options);
        rb_ary_push(m_sexp, regexp);
    }

    virtual void append_sexp(std::function<void(Creator *)> fn) override {
        MRICreator creator { *this };
        fn(&creator);
        rb_ary_push(m_sexp, creator.sexp());
    }

    virtual void append_string(TM::String &string) override {
        auto encoding = string.contains_seemingly_valid_utf8_encoded_characters() ? rb_utf8_encoding() : rb_ascii8bit_encoding();
        rb_ary_push(m_sexp, rb_enc_str_new(string.c_str(), string.length(), encoding));
    }

    virtual void append_symbol(TM::String &name) override {
        auto encoding = name.contains_utf8_encoded_multibyte_characters() ? rb_utf8_encoding() : rb_ascii8bit_encoding();
        auto symbol = ID2SYM(rb_intern3(name.c_str(), name.size(), encoding));
        rb_ary_push(m_sexp, symbol);
    }

    virtual void append_true() override {
        rb_ary_push(m_sexp, Qtrue);
    }

    virtual void make_complex_number() override {
        auto num = rb_ary_pop(m_sexp);
        num = rb_Complex(INT2FIX(0), num);
        rb_ary_push(m_sexp, num);
    }

    virtual void make_rational_number() override {
        auto num = rb_ary_pop(m_sexp);
        if (TYPE(num) == T_FLOAT)
            num = rb_flt_rationalize(num);
        else
            num = rb_Rational(num, INT2FIX(1));
        rb_ary_push(m_sexp, num);
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

    static VALUE get_file_string(const String &file) {
        auto file_string = s_file_cache.get(file);
        if (!file_string) {
            file_string = rb_str_new(file.c_str(), file.length());
            // FIXME: Seems there is no way to un-register and object. :-(
            rb_gc_register_mark_object(file_string);
            s_file_cache.put(file, file_string);
        }
        return file_string;
    }

    // TODO: Move this to the Parser object, pass it in, clean it up when finished with it.
    // (Otherwise we leak memory if the user parses lots of different files in a long-running process.)
    inline static TM::Hashmap<const String, VALUE> s_file_cache { TM::HashType::TMString };
};
}
