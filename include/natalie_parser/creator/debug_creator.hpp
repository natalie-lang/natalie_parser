#include "natalie_parser/creator.hpp"
#include "natalie_parser/node.hpp"

namespace NatalieParser {

class DebugCreator : public Creator {
public:
    DebugCreator(const Node *) { }

    virtual ~DebugCreator() { }

    virtual void set_comments(const TM::String &) override {
        // ignore for now
    }

    virtual void set_type(const char *type) override {
        if (m_nodes.size() >= 1)
            m_nodes[0] = String::format(":{}", type);
        else
            m_nodes.push_front(String::format(":{}", type));
    }

    virtual void append(const Node &node) override {
        if (node.type() == Node::Type::Nil) {
            m_nodes.push("nil");
            return;
        }
        DebugCreator creator { &node };
        creator.set_assignment(assignment());
        node.transform(&creator);
        m_nodes.push(creator.to_string());
    }

    virtual void append_array(const ArrayNode &array) override {
        DebugCreator creator { &array };
        creator.set_assignment(assignment());
        array.ArrayNode::transform(&creator);
        m_nodes.push(creator.to_string());
    }

    virtual void append_false() override {
        m_nodes.push("false");
    }

    virtual void append_float(double number) override {
        m_nodes.push(String(number));
    }

    virtual void append_integer(long long number) override {
        m_nodes.push(String(number));
    }

    virtual void append_integer(TM::String &number) override {
        m_nodes.push(String(number));
    }

    virtual void append_nil() override {
        m_nodes.push("nil");
    }

    virtual void append_range(long long first, long long last, bool exclude_end) override {
        m_nodes.push(String(first));
        m_nodes.push(exclude_end ? "..." : "..");
        m_nodes.push(String(last));
    }

    virtual void append_regexp(TM::String &pattern, int options) override {
        TM_UNUSED(options);
        m_nodes.push('/');
        m_nodes.push(pattern);
        m_nodes.push('/');
    }

    virtual void append_sexp(std::function<void(Creator *)> fn) override {
        DebugCreator creator { nullptr };
        fn(&creator);
        m_nodes.push(creator.to_string());
    }

    virtual void append_string(TM::String &string) override {
        m_nodes.push(String::format("\"{}\"", string));
    }

    virtual void append_symbol(TM::String &name) override {
        m_nodes.push(String::format(":{}", name));
    }

    virtual void append_true() override {
        m_nodes.push("true");
    }

    virtual void wrap(const char *type) override {
        auto inner = to_string();
        m_nodes.clear();
        set_type(type);
        m_nodes.push(inner);
    }

    TM::String to_string() {
        TM::String buf = "(";
        for (size_t i = 0; i < m_nodes.size(); ++i) {
            buf.append(m_nodes[i]);
            if (i + 1 < m_nodes.size())
                buf.append(", ");
        }
        buf.append_char(')');
        return buf;
    }

private:
    TM::Vector<TM::String> m_nodes {};
};
}
