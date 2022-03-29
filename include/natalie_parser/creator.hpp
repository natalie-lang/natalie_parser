#pragma once

#include "tm/shared_ptr.hpp"
#include "tm/string.hpp"
#include "tm/vector.hpp"
#include <functional>
#include <initializer_list>

namespace NatalieParser {

class Node;
class ArrayNode;

class Creator {
public:
    Creator() { }
    virtual void set_comments(const TM::String &comments) = 0;
    virtual void set_type(const char *type) = 0;
    virtual void append(const Node *node) { append(*node); }
    virtual void append(const Node &node) = 0;
    virtual void append_array(const ArrayNode *array) { append_array(*array); }
    virtual void append_array(const ArrayNode &array) = 0;
    virtual void append_false() = 0;
    virtual void append_float(double number) = 0;
    virtual void append_integer(long long number) = 0;
    virtual void append_integer(TM::String &number) = 0;
    virtual void append_nil() = 0;
    virtual void append_range(long long first, long long last, bool exclude_end) = 0;
    virtual void append_regexp(TM::String &pattern, int options) = 0;
    virtual void append_sexp(std::function<void(Creator *)> fn) = 0;
    virtual void append_string(TM::String &string) = 0;
    virtual void append_symbol(TM::String &symbol) = 0;
    virtual void append_true() = 0;
    virtual void wrap(const char *type) = 0;

    virtual ~Creator() { }

    void append_nil_sexp() {
        append_sexp([&](Creator *c) { c->set_type("nil"); });
    }

    void append_regexp(TM::SharedPtr<TM::String> pattern_ptr, int options) {
        if (!pattern_ptr) {
            auto p = TM::String("");
            append_regexp(p, options);
        }
        append_regexp(*pattern_ptr, options);
    }

    void append_string(TM::SharedPtr<TM::String> string_ptr) {
        if (!string_ptr) {
            auto s = TM::String("");
            append_string(s);
        }
        append_string(*string_ptr);
    }

    void append_symbol(TM::SharedPtr<TM::String> symbol_ptr) {
        if (!symbol_ptr) {
            auto s = TM::String("");
            append_symbol(s);
        }
        append_symbol(*symbol_ptr);
    }

    bool assignment() { return m_assignment; }
    void set_assignment(bool assignment) { m_assignment = assignment; }

    void with_assignment(bool assignment, std::function<void()> fn) {
        auto assignment_was = m_assignment;
        m_assignment = assignment;
        fn();
        m_assignment = assignment_was;
    }

private:
    bool m_assignment { false };
};

}
