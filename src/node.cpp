#include "natalie_parser/node.hpp"

namespace NatalieParser {

AliasNode::~AliasNode() {
    delete m_new_name;
    delete m_existing_name;
}

void AliasNode::transform(Creator *creator) const {
    creator->set_type("alias");
    creator->append(m_new_name);
    creator->append(m_existing_name);
}

void AssignmentNode::transform(Creator *creator) const {
    switch (identifier()->type()) {
    case Node::Type::MultipleAssignment: {
        auto masgn = static_cast<const MultipleAssignmentNode *>(m_identifier);
        masgn->transform(creator);
        creator->append(m_value);
        break;
    }
    case Node::Type::Call:
    case Node::Type::Colon2:
    case Node::Type::Colon3:
    case Node::Type::Identifier: {
        creator->with_assignment(true, [&]() {
            identifier()->transform(creator);
        });
        creator->append(m_value);
        break;
    }
    default:
        TM_UNREACHABLE();
    }
}

BeginNode::~BeginNode() {
    delete m_body;
    delete m_else_body;
    delete m_ensure_body;
    for (auto node : m_rescue_nodes)
        delete node;
}

BeginRescueNode::~BeginRescueNode() {
    delete m_name;
    delete m_body;
    for (auto node : m_exceptions)
        delete node;
}

void BeginNode::transform(Creator *creator) const {
    assert(m_body);
    creator->set_type("rescue");
    if (!m_body->is_empty())
        creator->append(m_body->without_unnecessary_nesting());
    for (auto rescue_node : m_rescue_nodes) {
        creator->append(rescue_node);
    }
    if (m_else_body)
        creator->append(m_else_body->without_unnecessary_nesting());
    if (m_ensure_body) {
        if (m_rescue_nodes.is_empty())
            creator->set_type("ensure");
        else
            creator->wrap("ensure");
        creator->append(m_ensure_body->without_unnecessary_nesting());
    }
}

Node *BeginRescueNode::name_to_node() const {
    assert(m_name);
    return new AssignmentNode {
        token(),
        m_name,
        new IdentifierNode {
            Token { Token::Type::GlobalVariable, "$!", file(), line(), column() },
            false },
    };
}

void BeginRescueNode::transform(Creator *creator) const {
    creator->set_type("resbody");
    auto array = new ArrayNode { token() };
    for (auto exception_node : m_exceptions) {
        array->add_node(exception_node);
    }
    if (m_name)
        array->add_node(name_to_node());
    creator->append(array);
    for (auto node : m_body->nodes())
        creator->append(node);
}

ClassNode::~ClassNode() {
    delete m_name;
    delete m_superclass;
    delete m_body;
}

void ClassNode::transform(Creator *creator) const {
    creator->set_type("class");
    if (m_name->type() == Node::Type::Identifier) {
        auto identifier = static_cast<IdentifierNode *>(m_name);
        creator->append_symbol(identifier->name());
    } else {
        creator->append(m_name);
    }
    creator->append(m_superclass);
    for (auto node : m_body->nodes())
        creator->append(node);
}

void InterpolatedShellNode::transform(Creator *creator) const {
    creator->set_type("dxstr");
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        auto node = m_nodes.at(i);
        if (i == 0 && node->type() == Node::Type::String) {
            auto string_node = static_cast<StringNode *>(node);
            creator->append_string(string_node->string());
        } else {
            creator->append(node);
        }
    }
}

void InterpolatedStringNode::transform(Creator *creator) const {
    creator->set_type("dstr");
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        auto node = m_nodes.at(i);
        if (i == 0 && node->type() == Node::Type::String) {
            auto string_node = static_cast<StringNode *>(node);
            creator->append_string(string_node->string());
        } else {
            creator->append(node);
        }
    }
}

void InterpolatedRegexpNode::transform(Creator *creator) const {
    creator->set_type("dregx");
    for (size_t i = 0; i < nodes().size(); ++i) {
        auto node = nodes()[i];
        if (i == 0 && node->type() == Node::Type::String)
            creator->append_string(static_cast<StringNode *>(node)->string());
        else
            creator->append(node);
    }
    if (m_options != 0)
        creator->append_integer(m_options);
}

MatchNode::~MatchNode() {
    delete m_regexp;
    delete m_arg;
}

void MatchNode::transform(Creator *creator) const {
    if (m_regexp_on_left)
        creator->set_type("match2");
    else
        creator->set_type("match3");
    creator->append(m_regexp);
    creator->append(m_arg);
}

void ModuleNode::transform(Creator *creator) const {
    creator->set_type("module");
    if (m_name->type() == Node::Type::Identifier) {
        auto identifier = static_cast<IdentifierNode *>(m_name);
        creator->append_symbol(identifier->name());
    } else {
        creator->append(m_name);
    }
    for (auto node : m_body->nodes())
        creator->append(node);
}

void MultipleAssignmentNode::add_locals(TM::Hashmap<const char *> &locals) {
    for (auto node : m_nodes) {
        switch (node->type()) {
        case Node::Type::Identifier: {
            auto identifier = static_cast<IdentifierNode *>(node);
            identifier->add_to_locals(locals);
            break;
        }
        case Node::Type::Call:
        case Node::Type::Colon2:
        case Node::Type::Colon3:
            break;
        case Node::Type::Splat: {
            auto splat = static_cast<SplatNode *>(node);
            if (splat->node() && splat->node()->type() == Node::Type::Identifier) {
                auto identifier = static_cast<IdentifierNode *>(splat->node());
                identifier->add_to_locals(locals);
            }
            break;
        }
        case Node::Type::MultipleAssignment:
            static_cast<MultipleAssignmentNode *>(node)->add_locals(locals);
            break;
        default:
            TM_UNREACHABLE();
        }
    }
}

void NodeWithArgs::append_method_or_block_args(Creator *creator) const {
    creator->append_sexp([&](Creator *c) {
        c->set_type("args");
        for (auto arg : m_args) {
            switch (arg->type()) {
            case Node::Type::Arg: {
                auto arg_node = static_cast<ArgNode *>(arg);
                if (arg_node->value())
                    c->append(arg);
                else
                    static_cast<ArgNode *>(arg)->append_name(c);
                break;
            }
            case Node::Type::KeywordArg:
            case Node::Type::MultipleAssignmentArg:
                c->append(arg);
                break;
            default:
                TM_UNREACHABLE();
            }
        }
    });
}

}
