#include "extconf.h"
#include "ruby.h"
#include "ruby/intern.h"
#include "stdio.h"

// this includes MUST come after
#include "natalie_parser/parser.hpp"

VALUE Parser;
VALUE Sexp;

VALUE s(std::initializer_list<VALUE> list) {
    auto sexp = rb_class_new_instance(0, nullptr, Sexp);
    for (auto item : list)
        rb_ary_push(sexp, item);
    return sexp;
}

VALUE str(const TM::String string) {
    return rb_str_new_cstr(string.c_str());
}

VALUE sym(const TM::String string) {
    return ID2SYM(rb_intern(string.c_str()));
}

VALUE sym(const char *string) {
    return ID2SYM(rb_intern(string));
}

extern "C" {

VALUE build_args_sexp(TM::Vector<NatalieParser::Node *> &args);
VALUE build_assignment_sexp(NatalieParser::Node *identifier);
VALUE multiple_assignment_to_ruby_with_array(NatalieParser::MultipleAssignmentNode *node);
VALUE class_or_module_name_to_ruby(NatalieParser::Node *name);

VALUE to_ruby(NatalieParser::Node *node) {
    switch (node->type()) {
    case NatalieParser::Node::Type::Alias: {
        auto alias_node = static_cast<NatalieParser::AliasNode *>(node);
        return s({
            sym("alias"),
            to_ruby(alias_node->new_name()),
            to_ruby(alias_node->existing_name()),
        });
    }
    case NatalieParser::Node::Type::Arg: {
        auto arg_node = static_cast<NatalieParser::ArgNode *>(node);
        if (arg_node->value()) {
            return s({
                sym("lasgn"),
                sym(arg_node->name().ref()),
                to_ruby(arg_node->value()),
            });
        } else {
            TM::String name;
            if (arg_node->name())
                name = TM::String(arg_node->name().ref());
            else
                name = TM::String();
            if (arg_node->splat()) {
                name.prepend_char('*');
            } else if (arg_node->kwsplat()) {
                name.prepend_char('*');
                name.prepend_char('*');
            } else if (arg_node->block_arg()) {
                name.prepend_char('&');
            }
            return sym(name);
        }
    }
    case NatalieParser::Node::Type::Array: {
        auto array_node = static_cast<NatalieParser::ArrayNode *>(node);
        auto sexp = s({ sym("array") });
        for (auto item_node : array_node->nodes()) {
            rb_ary_push(sexp, to_ruby(item_node));
        }
        return sexp;
    }
    case NatalieParser::Node::Type::ArrayPattern: {
        auto array_node = static_cast<NatalieParser::ArrayNode *>(node);
        auto sexp = s({ sym("array_pat") });
        if (!array_node->nodes().is_empty())
            rb_ary_push(sexp, Qnil); // NOTE: I don't know what this nil is for
        for (auto item_node : array_node->nodes()) {
            rb_ary_push(sexp, to_ruby(item_node));
        }
        return sexp;
    }
    case NatalieParser::Node::Type::Assignment: {
        auto assignment_node = static_cast<NatalieParser::AssignmentNode *>(node);
        switch (assignment_node->identifier()->type()) {
        case NatalieParser::Node::Type::MultipleAssignment: {
            auto masgn = static_cast<NatalieParser::MultipleAssignmentNode *>(assignment_node->identifier());
            auto sexp = multiple_assignment_to_ruby_with_array(masgn);
            auto value = to_ruby(assignment_node->value());
            rb_ary_push(sexp, value);
            return sexp;
        }
        case NatalieParser::Node::Type::Call:
        case NatalieParser::Node::Type::Colon2:
        case NatalieParser::Node::Type::Colon3:
        case NatalieParser::Node::Type::Identifier: {
            auto sexp = build_assignment_sexp(assignment_node->identifier());
            rb_ary_push(sexp, to_ruby(assignment_node->value()));
            return sexp;
        }
        default:
            TM_UNREACHABLE();
        }
    }
    case NatalieParser::Node::Type::Begin: {
        auto begin_node = static_cast<NatalieParser::BeginNode *>(node);
        assert(begin_node->body());
        auto sexp = s({ sym("rescue") });
        if (!begin_node->body()->is_empty())
            rb_ary_push(sexp, to_ruby(begin_node->body()->without_unnecessary_nesting()));
        for (auto rescue_node : begin_node->rescue_nodes()) {
            rb_ary_push(sexp, to_ruby(rescue_node));
        }
        if (begin_node->else_body())
            rb_ary_push(sexp, to_ruby(begin_node->else_body()->without_unnecessary_nesting()));
        if (begin_node->ensure_body()) {
            if (begin_node->rescue_nodes().is_empty())
                rb_ary_store(sexp, 0, sym("ensure"));
            else
                sexp = s({ sym("ensure"), sexp });
            rb_ary_push(sexp, to_ruby(begin_node->ensure_body()->without_unnecessary_nesting()));
        }
        return sexp;
    }
    case NatalieParser::Node::Type::BeginRescue: {
        auto begin_rescue_node = static_cast<NatalieParser::BeginRescueNode *>(node);
        auto array = new NatalieParser::ArrayNode { begin_rescue_node->token() };
        for (auto exception_node : begin_rescue_node->exceptions()) {
            array->add_node(exception_node);
        }
        if (begin_rescue_node->name())
            array->add_node(begin_rescue_node->name_to_node());
        auto rescue_node = s({
            sym("resbody"),
            to_ruby(array),
        });
        for (auto node : begin_rescue_node->body()->nodes()) {
            rb_ary_push(rescue_node, to_ruby(node));
        }
        return rescue_node;
    }
    case NatalieParser::Node::Type::Block: {
        auto block_node = static_cast<NatalieParser::BlockNode *>(node);
        auto array = s({ sym("block") });
        for (auto item_node : block_node->nodes()) {
            rb_ary_push(array, to_ruby(item_node));
        }
        return array;
    }
    case NatalieParser::Node::Type::BlockPass: {
        auto block_pass_node = static_cast<NatalieParser::BlockPassNode *>(node);
        auto sexp = s({ sym("block_pass") });
        rb_ary_push(sexp, to_ruby(block_pass_node->node()));
        return sexp;
    }
    case NatalieParser::Node::Type::Break: {
        auto break_node = static_cast<NatalieParser::BreakNode *>(node);
        auto sexp = s({ sym("break") });
        if (break_node->arg())
            rb_ary_push(sexp, to_ruby(break_node->arg()));
        return sexp;
    }
    case NatalieParser::Node::Type::Call: {
        auto call_node = static_cast<NatalieParser::CallNode *>(node);
        auto sexp = s({
            sym("call"),
            to_ruby(call_node->receiver()),
            sym(call_node->message().ref()),
        });
        for (auto arg : call_node->args()) {
            rb_ary_push(sexp, to_ruby(arg));
        }
        return sexp;
    }
    case NatalieParser::Node::Type::Case: {
        auto case_node = static_cast<NatalieParser::CaseNode *>(node);
        auto sexp = s({
            sym("case"),
            to_ruby(case_node->subject()),
        });
        for (auto when_node : case_node->nodes()) {
            rb_ary_push(sexp, to_ruby(when_node));
        }
        if (case_node->else_node()) {
            rb_ary_push(sexp, to_ruby(case_node->else_node()->without_unnecessary_nesting()));
        } else {
            rb_ary_push(sexp, Qnil);
        }
        return sexp;
    }
    case NatalieParser::Node::Type::CaseIn: {
        auto case_in_node = static_cast<NatalieParser::CaseInNode *>(node);
        auto sexp = s({
            sym("in"),
            to_ruby(case_in_node->pattern()),
        });
        for (auto node : case_in_node->body()->nodes()) {
            rb_ary_push(sexp, to_ruby(node));
        }
        return sexp;
    }
    case NatalieParser::Node::Type::CaseWhen: {
        auto case_when_node = static_cast<NatalieParser::CaseWhenNode *>(node);
        auto sexp = s({
            sym("when"),
            to_ruby(case_when_node->condition()),
        });
        for (auto node : case_when_node->body()->nodes()) {
            rb_ary_push(sexp, to_ruby(node));
        }
        return sexp;
    }
    case NatalieParser::Node::Type::Class: {
        auto class_node = static_cast<NatalieParser::ClassNode *>(node);
        auto sexp = s({ sym("class"), class_or_module_name_to_ruby(class_node->name()), to_ruby(class_node->superclass()) });
        if (!class_node->body()->is_empty()) {
            for (auto node : class_node->body()->nodes()) {
                rb_ary_push(sexp, to_ruby(node));
            }
        }
        return sexp;
    }
    case NatalieParser::Node::Type::Colon2: {
        auto colon2_node = static_cast<NatalieParser::Colon2Node *>(node);
        return s({
            sym("colon2"),
            to_ruby(colon2_node->left()),
            sym(colon2_node->name().ref()),
        });
    }
    case NatalieParser::Node::Type::Colon3: {
        auto colon3_node = static_cast<NatalieParser::Colon3Node *>(node);
        return s({
            sym("colon3"),
            sym(colon3_node->name().ref()),
        });
    }
    case NatalieParser::Node::Type::Constant: {
        auto constant_node = static_cast<NatalieParser::ConstantNode *>(node);
        return s({
            sym("const"),
            sym(*constant_node->token().literal_string()),
        });
    }
    case NatalieParser::Node::Type::Defined: {
        auto defined_node = static_cast<NatalieParser::DefinedNode *>(node);
        return s({
            sym("defined"),
            to_ruby(defined_node->arg()),
        });
    }
    case NatalieParser::Node::Type::Def: {
        auto def_node = static_cast<NatalieParser::DefNode *>(node);
        VALUE sexp;
        if (def_node->self_node()) {
            sexp = s({
                sym("defs"),
                to_ruby(def_node->self_node()),
                sym(*def_node->name()),
                build_args_sexp(def_node->args()),
            });
        } else {
            sexp = s({
                sym("defn"),
                sym(*def_node->name()),
                build_args_sexp(def_node->args()),
            });
        }
        if (def_node->body()->is_empty()) {
            rb_ary_push(sexp, s({ sym("nil") }));
        } else {
            for (auto node : def_node->body()->nodes()) {
                rb_ary_push(sexp, to_ruby(node));
            }
        }
        return sexp;
    }
    case NatalieParser::Node::Type::EvaluateToString: {
        auto evaluate_to_string_node = static_cast<NatalieParser::EvaluateToStringNode *>(node);
        return s({ sym("evstr"), to_ruby(evaluate_to_string_node->node()) });
    }
    case NatalieParser::Node::Type::False: {
        auto false_node = static_cast<NatalieParser::FalseNode *>(node);
        return s({ sym("false") });
    }
    case NatalieParser::Node::Type::Hash: {
        auto hash_node = static_cast<NatalieParser::HashNode *>(node);
        auto sexp = s({ sym("hash") });
        for (auto node : hash_node->nodes()) {
            rb_ary_push(sexp, to_ruby(node));
        }
        return sexp;
    }
    case NatalieParser::Node::Type::HashPattern: {
        auto hash_node = static_cast<NatalieParser::HashNode *>(node);
        auto sexp = s({ sym("hash_pat") });
        rb_ary_push(sexp, Qnil); // NOTE: I don't know what this nil is for
        for (auto node : hash_node->nodes()) {
            rb_ary_push(sexp, to_ruby(node));
        }
        return sexp;
    }
    case NatalieParser::Node::Type::Identifier: {
        auto identifier_node = static_cast<NatalieParser::IdentifierNode *>(node);
        switch (identifier_node->token_type()) {
        case NatalieParser::Token::Type::BareName:
            if (identifier_node->is_lvar()) {
                return s({ sym("lvar"), sym(identifier_node->name().ref()) });
            } else {
                return s({ sym("call"), Qnil, sym(identifier_node->name().ref()) });
            }
        case NatalieParser::Token::Type::ClassVariable:
            return s({ sym("cvar"), sym(identifier_node->name().ref()) });
        case NatalieParser::Token::Type::Constant:
            return s({ sym("const"), sym(identifier_node->name().ref()) });
        case NatalieParser::Token::Type::GlobalVariable: {
            auto ref = identifier_node->nth_ref();
            if (ref > 0)
                return s({ sym("nth_ref"), rb_int_new(ref) });
            else
                return s({ sym("gvar"), sym(identifier_node->name().ref()) });
        }
        case NatalieParser::Token::Type::InstanceVariable:
            return s({ sym("ivar"), sym(identifier_node->name().ref()) });
        default:
            TM_NOT_YET_IMPLEMENTED();
        }
    }
    case NatalieParser::Node::Type::If: {
        auto if_node = static_cast<NatalieParser::IfNode *>(node);
        return s({
            sym("if"),
            to_ruby(if_node->condition()),
            to_ruby(if_node->true_expr()),
            to_ruby(if_node->false_expr()),
        });
    }
    case NatalieParser::Node::Type::Iter: {
        auto iter_node = static_cast<NatalieParser::IterNode *>(node);
        auto sexp = s({
            sym("iter"),
            to_ruby(iter_node->call()),
        });
        if (iter_node->args().is_empty())
            rb_ary_push(sexp, rb_int_new(0));
        else
            rb_ary_push(sexp, build_args_sexp(iter_node->args()));
        if (!iter_node->body()->is_empty()) {
            if (iter_node->body()->has_one_node())
                rb_ary_push(sexp, to_ruby(iter_node->body()->nodes()[0]));
            else
                rb_ary_push(sexp, to_ruby(iter_node->body()));
        }
        return sexp;
    }
    case NatalieParser::Node::Type::InterpolatedRegexp: {
        auto interpolated_regexp_node = static_cast<NatalieParser::InterpolatedRegexpNode *>(node);
        auto sexp = s({ sym("dregx") });
        for (size_t i = 0; i < interpolated_regexp_node->nodes().size(); i++) {
            auto node = interpolated_regexp_node->nodes()[i];
            rb_ary_push(sexp, to_ruby(node));
        }
        if (interpolated_regexp_node->options() != 0)
            rb_ary_push(sexp, rb_int_new(interpolated_regexp_node->options()));
        return sexp;
    }
    case NatalieParser::Node::Type::InterpolatedShell: {
        auto interpolated_shell_node = static_cast<NatalieParser::InterpolatedShellNode *>(node);
        auto sexp = s({ sym("dxstr") });
        for (size_t i = 0; i < interpolated_shell_node->nodes().size(); i++) {
            auto node = interpolated_shell_node->nodes()[i];
            rb_ary_push(sexp, to_ruby(node));
        }
        return sexp;
    }
    case NatalieParser::Node::Type::InterpolatedString: {
        auto interpolated_string_node = static_cast<NatalieParser::InterpolatedStringNode *>(node);
        auto sexp = s({ sym("dstr") });
        for (size_t i = 0; i < interpolated_string_node->nodes().size(); i++) {
            auto node = interpolated_string_node->nodes()[i];
            rb_ary_push(sexp, to_ruby(node));
        }
        return sexp;
    }
    case NatalieParser::Node::Type::KeywordArg: {
        auto keyword_arg_node = static_cast<NatalieParser::KeywordArgNode *>(node);
        auto sexp = s({
            sym("kwarg"),
            sym(*keyword_arg_node->name()),
        });
        if (keyword_arg_node->value())
            rb_ary_push(sexp, to_ruby(keyword_arg_node->value()));
        return sexp;
    }
    case NatalieParser::Node::Type::KeywordSplat: {
        auto keyword_splat_node = static_cast<NatalieParser::KeywordSplatNode *>(node);
        auto sexp = s({ sym("hash") });
        auto kwsplat_sexp = s({ sym("kwsplat") });
        if (keyword_splat_node->node())
            rb_ary_push(kwsplat_sexp, to_ruby(keyword_splat_node->node()));
        rb_ary_push(sexp, kwsplat_sexp);
        return sexp;
    }
    case NatalieParser::Node::Type::Integer: {
        auto integer_node = static_cast<NatalieParser::IntegerNode *>(node);
        return s({ sym("lit"), rb_int_new(integer_node->number()) });
    }
    case NatalieParser::Node::Type::Float: {
        auto float_node = static_cast<NatalieParser::FloatNode *>(node);
        return s({ sym("lit"), rb_float_new(float_node->number()) });
    }
    case NatalieParser::Node::Type::LogicalAnd: {
        auto logical_and_node = static_cast<NatalieParser::LogicalAndNode *>(node);
        auto sexp = s({
            sym("and"),
            to_ruby(logical_and_node->left()),
            to_ruby(logical_and_node->right()),
        });

        return sexp;
    }
    case NatalieParser::Node::Type::LogicalOr: {
        auto logical_or_node = static_cast<NatalieParser::LogicalOrNode *>(node);
        auto sexp = s({
            sym("or"),
            to_ruby(logical_or_node->left()),
            to_ruby(logical_or_node->right()),
        });

        return sexp;
    }
    case NatalieParser::Node::Type::Match: {
        auto match_node = static_cast<NatalieParser::MatchNode *>(node);
        return s({
            match_node->regexp_on_left() ? sym("match2") : sym("match3"),
            to_ruby(match_node->regexp()),
            to_ruby(match_node->arg()),
        });
    }
    case NatalieParser::Node::Type::Module: {
        auto module_node = static_cast<NatalieParser::ModuleNode *>(node);
        auto sexp = s({ sym("module"), class_or_module_name_to_ruby(module_node->name()) });
        if (!module_node->body()->is_empty()) {
            for (auto node : module_node->body()->nodes()) {
                rb_ary_push(sexp, to_ruby(node));
            }
        }
        return sexp;
    }
    case NatalieParser::Node::Type::MultipleAssignment: {
        auto multiple_assignment_node = static_cast<NatalieParser::MultipleAssignmentNode *>(node);
        auto sexp = s({ sym("masgn") });
        for (auto node : multiple_assignment_node->nodes()) {
            switch (node->type()) {
            case NatalieParser::Node::Type::Arg:
            case NatalieParser::Node::Type::MultipleAssignment:
                rb_ary_push(sexp, to_ruby(node));
                break;
            default:
                TM_NOT_YET_IMPLEMENTED(); // maybe not needed?
            }
        }
        return sexp;
    }
    case NatalieParser::Node::Type::Next: {
        auto next_node = static_cast<NatalieParser::NextNode *>(node);
        auto sexp = s({ sym("next") });
        if (next_node->arg())
            rb_ary_push(sexp, to_ruby(next_node->arg()));
        return sexp;
    }
    case NatalieParser::Node::Type::Nil: {
        auto nil_node = static_cast<NatalieParser::NilNode *>(node);
        return Qnil;
    }
    case NatalieParser::Node::Type::NilSexp: {
        auto nil_sexp_node = static_cast<NatalieParser::NilSexpNode *>(node);
        return s({ sym("nil") });
    }
    case NatalieParser::Node::Type::Not: {
        auto not_node = static_cast<NatalieParser::NotNode *>(node);
        return s({ sym("not"), to_ruby(not_node->expression()) });
    }
    case NatalieParser::Node::Type::OpAssign: {
        auto op_assign_node = static_cast<NatalieParser::OpAssignNode *>(node);
        auto sexp = build_assignment_sexp(op_assign_node->name());
        assert(op_assign_node->op());
        auto call = new NatalieParser::CallNode { op_assign_node->token(), op_assign_node->name(), op_assign_node->op() };
        call->add_arg(op_assign_node->value());
        rb_ary_push(sexp, to_ruby(call));
        return sexp;
    }
    case NatalieParser::Node::Type::OpAssignAccessor: {
        auto op_assign_accessor_node = static_cast<NatalieParser::OpAssignAccessorNode *>(node);
        if (*op_assign_accessor_node->message() == "[]=") {
            auto arg_list = s({ sym("arglist") });
            for (auto arg : op_assign_accessor_node->args()) {
                rb_ary_push(arg_list, to_ruby(arg));
            }
            return s({
                sym("op_asgn1"),
                to_ruby(op_assign_accessor_node->receiver()),
                arg_list,
                sym(*op_assign_accessor_node->op()),
                to_ruby(op_assign_accessor_node->value()),
            });
        } else {
            assert(op_assign_accessor_node->args().is_empty());
            return s({
                sym("op_asgn2"),
                to_ruby(op_assign_accessor_node->receiver()),
                sym(*op_assign_accessor_node->message()),
                sym(*op_assign_accessor_node->op()),
                to_ruby(op_assign_accessor_node->value()),
            });
        }
    }
    case NatalieParser::Node::Type::OpAssignAnd: {
        auto op_assign_and_node = static_cast<NatalieParser::OpAssignAndNode *>(node);
        auto assignment_node = new NatalieParser::AssignmentNode {
            op_assign_and_node->token(),
            op_assign_and_node->name(),
            op_assign_and_node->value()
        };
        return s({
            sym("op_asgn_and"),
            to_ruby(op_assign_and_node->name()),
            to_ruby(assignment_node),
        });
    }
    case NatalieParser::Node::Type::OpAssignOr: {
        auto op_assign_or_node = static_cast<NatalieParser::OpAssignOrNode *>(node);
        auto assignment_node = new NatalieParser::AssignmentNode {
            op_assign_or_node->token(),
            op_assign_or_node->name(),
            op_assign_or_node->value()
        };
        return s({
            sym("op_asgn_or"),
            to_ruby(op_assign_or_node->name()),
            to_ruby(assignment_node),
        });
    }
    case NatalieParser::Node::Type::Pin: {
        auto pin_node = static_cast<NatalieParser::PinNode *>(node);
        auto sexp = s({ sym("pin") });
        rb_ary_push(sexp, to_ruby(pin_node->identifier()));
        return sexp;
    }
    case NatalieParser::Node::Type::Range: {
        auto range_node = static_cast<NatalieParser::RangeNode *>(node);
        return s({ range_node->exclude_end() ? sym("dot3") : sym("dot2"),
            to_ruby(range_node->first()),
            to_ruby(range_node->last()) });
    }
    case NatalieParser::Node::Type::Regexp: {
        auto regexp_node = static_cast<NatalieParser::RegexpNode *>(node);
        auto regexp = rb_reg_new(regexp_node->pattern()->c_str(), regexp_node->pattern()->size(), regexp_node->options());
        return s({ sym("lit"), regexp });
    }
    case NatalieParser::Node::Type::Return: {
        auto return_node = static_cast<NatalieParser::ReturnNode *>(node);
        auto sexp = s({ sym("return") });
        auto value = return_node->value();
        if (value->type() != NatalieParser::Node::Type::Nil)
            rb_ary_push(sexp, to_ruby(value));
        return sexp;
    }
    case NatalieParser::Node::Type::SafeCall: {
        auto safe_call_node = static_cast<NatalieParser::SafeCallNode *>(node);
        auto sexp = s({
            sym("safe_call"),
            to_ruby(safe_call_node->receiver()),
            sym(*safe_call_node->message()),
        });

        for (auto arg : safe_call_node->args()) {
            rb_ary_push(sexp, to_ruby(arg));
        }
        return sexp;
    }
    case NatalieParser::Node::Type::Self: {
        auto self_node = static_cast<NatalieParser::SelfNode *>(node);
        return s({ sym("self") });
    }
    case NatalieParser::Node::Type::Sclass: {
        auto sclass_node = static_cast<NatalieParser::SclassNode *>(node);
        auto sexp = s({
            sym("sclass"),
            to_ruby(sclass_node->klass()),
        });
        for (auto node : sclass_node->body()->nodes()) {
            rb_ary_push(sexp, to_ruby(node));
        }
        return sexp;
    }
    case NatalieParser::Node::Type::Shell: {
        auto shell_node = static_cast<NatalieParser::ShellNode *>(node);
        return s({ sym("xstr"), str(shell_node->string().ref()) });
    }
    case NatalieParser::Node::Type::Splat: {
        auto splat_node = static_cast<NatalieParser::SplatNode *>(node);
        auto sexp = s({ sym("splat") });
        if (splat_node->node())
            rb_ary_push(sexp, to_ruby(splat_node->node()));
        return sexp;
    }
    case NatalieParser::Node::Type::SplatValue: {
        auto splat_value_node = static_cast<NatalieParser::SplatValueNode *>(node);
        return s({ sym("svalue"), to_ruby(splat_value_node->value()) });
    }
    case NatalieParser::Node::Type::StabbyProc: {
        auto stabby_proc_node = static_cast<NatalieParser::StabbyProcNode *>(node);
        return s({ sym("lambda") });
    }
    case NatalieParser::Node::Type::String: {
        auto string_node = static_cast<NatalieParser::StringNode *>(node);
        return s({ sym("str"), str(*string_node->string()) });
    }
    case NatalieParser::Node::Type::Symbol: {
        auto symbol_node = static_cast<NatalieParser::SymbolNode *>(node);
        return s({ sym("lit"), sym(symbol_node->name().ref()) });
    }
    case NatalieParser::Node::Type::ToArray: {
        auto to_array_node = static_cast<NatalieParser::ToArrayNode *>(node);
        return s({ sym("to_ary"), to_ruby(to_array_node->value()) });
    }
    case NatalieParser::Node::Type::True: {
        auto true_node = static_cast<NatalieParser::TrueNode *>(node);
        return s({ sym("true") });
    }
    case NatalieParser::Node::Type::Super: {
        auto super_node = static_cast<NatalieParser::SuperNode *>(node);
        if (super_node->empty_parens()) {
            return s({ sym("super") });
        } else if (super_node->args().is_empty()) {
            return s({ sym("zsuper") });
        }
        auto sexp = s({ sym("super") });
        for (auto arg : super_node->args()) {
            rb_ary_push(sexp, to_ruby(arg));
        }
        return sexp;
    }
    case NatalieParser::Node::Type::Until:
    case NatalieParser::Node::Type::While: {
        auto while_node = static_cast<NatalieParser::WhileNode *>(node);
        VALUE is_pre, body;
        if (while_node->pre())
            is_pre = Qtrue;
        else
            is_pre = Qfalse;
        if (while_node->body()->is_empty())
            body = Qnil;
        else
            body = to_ruby(while_node->body()->without_unnecessary_nesting());
        return s({
            node->type() == NatalieParser::Node::Type::Until ? sym("until") : sym("while"),
            to_ruby(while_node->condition()),
            body,
            is_pre,
        });
    }
    case NatalieParser::Node::Type::Yield: {
        auto yield_node = static_cast<NatalieParser::YieldNode *>(node);
        auto sexp = s({ sym("yield") });
        if (yield_node->args().is_empty())
            return sexp;
        for (auto arg : yield_node->args()) {
            rb_ary_push(sexp, to_ruby(arg));
        }
        return sexp;
    }
    default:
        printf("Unknown Natalie Parser node type: %d\n", (int)node->type());
        return Qnil;
    }
}

VALUE build_args_sexp(TM::Vector<NatalieParser::Node *> &args) {
    auto sexp = s({ sym("args") });
    for (auto arg : args) {
        switch (arg->type()) {
        case NatalieParser::Node::Type::Arg:
        case NatalieParser::Node::Type::KeywordArg:
        case NatalieParser::Node::Type::MultipleAssignment:
            rb_ary_push(sexp, to_ruby(arg));
            break;
        default:
            TM_UNREACHABLE();
        }
    }
    return sexp;
}

VALUE build_assignment_sexp(NatalieParser::Node *identifier) {
    VALUE name;
    switch (identifier->type()) {
    case NatalieParser::Node::Type::Call: {
        auto call_node = static_cast<NatalieParser::CallNode *>(identifier);
        name = str(call_node->message().ref());
        rb_str_append(name, str("="));
        auto sexp = s({
            sym("attrasgn"),
            to_ruby(call_node->receiver()),
            sym(StringValueCStr(name)),
        });
        for (auto arg : call_node->args()) {
            rb_ary_push(sexp, to_ruby(arg));
        }
        return sexp;
    }
    case NatalieParser::Node::Type::Colon2:
    case NatalieParser::Node::Type::Colon3:
        name = to_ruby(identifier);
        break;
    case NatalieParser::Node::Type::Identifier:
        name = sym(static_cast<NatalieParser::IdentifierNode *>(identifier)->name().ref());
        break;
    case NatalieParser::Node::Type::Splat: {
        auto splat_node = static_cast<NatalieParser::SplatNode *>(identifier);
        auto sexp = s({ sym("splat") });
        if (splat_node->node())
            rb_ary_push(sexp, build_assignment_sexp(splat_node->node()));
        return sexp;
    }
    default:
        TM_NOT_YET_IMPLEMENTED("node type %d", (int)identifier->type());
    }
    VALUE type;
    switch (identifier->token().type()) {
    case NatalieParser::Token::Type::BareName:
        type = sym("lasgn");
        break;
    case NatalieParser::Token::Type::ClassVariable:
        type = sym("cvdecl");
        break;
    case NatalieParser::Token::Type::Constant:
    case NatalieParser::Token::Type::ConstantResolution:
        type = sym("cdecl");
        break;
    case NatalieParser::Token::Type::GlobalVariable:
        type = sym("gasgn");
        break;
    case NatalieParser::Token::Type::InstanceVariable:
        type = sym("iasgn");
        break;
    default:
        printf("got token type %d\n", (int)identifier->token().type());
        TM_UNREACHABLE();
    }
    return s({ type, name });
}

VALUE multiple_assignment_to_ruby_with_array(NatalieParser::MultipleAssignmentNode *node) {
    auto sexp = s({ sym("masgn") });
    auto array = s({ sym("array") });
    for (auto identifier : node->nodes()) {
        switch (identifier->type()) {
        case NatalieParser::Node::Type::Call:
        case NatalieParser::Node::Type::Colon2:
        case NatalieParser::Node::Type::Colon3:
        case NatalieParser::Node::Type::Identifier:
        case NatalieParser::Node::Type::Splat:
            rb_ary_push(array, build_assignment_sexp(identifier));
            break;
        case NatalieParser::Node::Type::MultipleAssignment:
            rb_ary_push(array, multiple_assignment_to_ruby_with_array(static_cast<NatalieParser::MultipleAssignmentNode *>(identifier)));
            break;
        default:
            TM_UNREACHABLE();
        }
    }
    rb_ary_push(sexp, array);
    return sexp;
}

VALUE class_or_module_name_to_ruby(NatalieParser::Node *name) {
    if (name->type() == NatalieParser::Node::Type::Identifier) {
        auto identifier = static_cast<NatalieParser::IdentifierNode *>(name);
        return sym(identifier->name().ref());
    } else {
        return to_ruby(name);
    }
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
