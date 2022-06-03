#pragma once

#include "natalie_parser/lexer.hpp"
#include "natalie_parser/node.hpp"
#include "natalie_parser/token.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class Parser {
public:
    class SyntaxError {
    public:
        SyntaxError(const char *message)
            : m_message { strdup(message) } {
            assert(m_message);
        }

        SyntaxError(const String &message)
            : SyntaxError { message.c_str() } { }

        ~SyntaxError() {
            free(m_message);
        }

        SyntaxError(const SyntaxError &) = delete;
        SyntaxError &operator=(const SyntaxError &) = delete;

        const char *message() { return m_message; }

    private:
        char *m_message { nullptr };
    };

    Parser(SharedPtr<String> code, SharedPtr<String> file)
        : m_code { code }
        , m_file { file } {
        m_tokens = Lexer { m_code, m_file }.tokens();
        m_call_depth.push(0);
    }

    ~Parser() {
        // SharedPtr ftw
    }

    using LocalsHashmap = TM::Hashmap<TM::String>;

    enum class Precedence;

    SharedPtr<Node> tree();

private:
    bool higher_precedence(Token &token, SharedPtr<Node> left, Precedence current_precedence);

    Precedence get_precedence(Token &token, SharedPtr<Node> left = {});

    bool is_first_arg_of_call_without_parens(SharedPtr<Node>, Token &);

    SharedPtr<Node> parse_expression(Precedence, LocalsHashmap &);

    SharedPtr<BlockNode> parse_body(LocalsHashmap &, Precedence, std::function<bool(Token::Type)>, bool = false);
    SharedPtr<BlockNode> parse_body(LocalsHashmap &, Precedence, Token::Type = Token::Type::EndKeyword, bool = false);
    SharedPtr<BlockNode> parse_case_in_body(LocalsHashmap &);
    SharedPtr<BlockNode> parse_case_when_body(LocalsHashmap &);
    SharedPtr<Node> parse_if_body(LocalsHashmap &);
    SharedPtr<BlockNode> parse_def_body(LocalsHashmap &);

    SharedPtr<Node> parse_alias(LocalsHashmap &);
    SharedPtr<SymbolNode> parse_alias_arg(LocalsHashmap &, const char *, bool);
    SharedPtr<Node> parse_array(LocalsHashmap &);
    SharedPtr<Node> parse_back_ref(LocalsHashmap &);
    SharedPtr<Node> parse_begin_block(LocalsHashmap &);
    SharedPtr<Node> parse_begin(LocalsHashmap &);
    void parse_rest_of_begin(BeginNode &, LocalsHashmap &);
    SharedPtr<Node> parse_beginless_range(LocalsHashmap &);
    SharedPtr<Node> parse_block_pass(LocalsHashmap &);
    SharedPtr<Node> parse_bool(LocalsHashmap &);
    SharedPtr<Node> parse_break(LocalsHashmap &);
    SharedPtr<Node> parse_class(LocalsHashmap &);
    SharedPtr<Node> parse_class_or_module_name(LocalsHashmap &);
    SharedPtr<Node> parse_case(LocalsHashmap &);
    SharedPtr<Node> parse_case_in_pattern(LocalsHashmap &);
    SharedPtr<Node> parse_case_in_patterns(LocalsHashmap &);
    void parse_comma_separated_expressions(ArrayNode &, LocalsHashmap &);
    SharedPtr<Node> parse_constant(LocalsHashmap &);
    SharedPtr<Node> parse_def(LocalsHashmap &);
    SharedPtr<Node> parse_defined(LocalsHashmap &);

    void parse_def_args(Vector<SharedPtr<Node>> &, LocalsHashmap &);
    enum class ArgsContext {
        Block,
        Method,
        Proc,
    };
    void parse_def_single_arg(Vector<SharedPtr<Node>> &, LocalsHashmap &, ArgsContext);

    SharedPtr<Node> parse_encoding(LocalsHashmap &);
    SharedPtr<Node> parse_end_block(LocalsHashmap &);
    SharedPtr<Node> parse_file_constant(LocalsHashmap &);
    SharedPtr<Node> parse_forward_args(LocalsHashmap &);
    SharedPtr<Node> parse_group(LocalsHashmap &);
    SharedPtr<Node> parse_hash(LocalsHashmap &);
    SharedPtr<Node> parse_hash_inner(LocalsHashmap &, Precedence, Token::Type, SharedPtr<Node> = {});
    SharedPtr<Node> parse_identifier(LocalsHashmap &);
    SharedPtr<Node> parse_if(LocalsHashmap &);
    void parse_interpolated_body(LocalsHashmap &, InterpolatedNode &, Token::Type);
    SharedPtr<Node> parse_interpolated_regexp(LocalsHashmap &);
    int parse_regexp_options(String &);
    SharedPtr<Node> parse_interpolated_shell(LocalsHashmap &);
    SharedPtr<Node> parse_interpolated_string(LocalsHashmap &);
    SharedPtr<Node> parse_interpolated_symbol(LocalsHashmap &);
    SharedPtr<Node> parse_lit(LocalsHashmap &);
    SharedPtr<Node> parse_keyword_splat(LocalsHashmap &);
    SharedPtr<Node> parse_keyword_splat_wrapped_in_hash(LocalsHashmap &);
    SharedPtr<String> parse_method_name(LocalsHashmap &);
    SharedPtr<Node> parse_module(LocalsHashmap &);
    SharedPtr<Node> parse_next(LocalsHashmap &);
    SharedPtr<Node> parse_nil(LocalsHashmap &);
    SharedPtr<Node> parse_not(LocalsHashmap &);
    SharedPtr<Node> parse_nth_ref(LocalsHashmap &);
    void parse_proc_args(Vector<SharedPtr<Node>> &, LocalsHashmap &);
    SharedPtr<Node> parse_redo(LocalsHashmap &);
    SharedPtr<Node> parse_retry(LocalsHashmap &);
    SharedPtr<Node> parse_return(LocalsHashmap &);
    SharedPtr<Node> parse_sclass(LocalsHashmap &);
    SharedPtr<Node> parse_self(LocalsHashmap &);
    void parse_shadow_variables_in_args(Vector<SharedPtr<Node>> &, LocalsHashmap &);
    SharedPtr<String> parse_shadow_variable_single_arg();
    SharedPtr<Node> parse_splat(LocalsHashmap &);
    SharedPtr<Node> parse_stabby_proc(LocalsHashmap &);
    SharedPtr<Node> parse_string(LocalsHashmap &);
    SharedPtr<Node> parse_super(LocalsHashmap &);
    SharedPtr<Node> parse_symbol(LocalsHashmap &);
    SharedPtr<Node> parse_symbol_key(LocalsHashmap &);
    SharedPtr<Node> parse_statement_keyword(LocalsHashmap &);
    SharedPtr<Node> parse_top_level_constant(LocalsHashmap &);
    SharedPtr<Node> parse_triple_dot(LocalsHashmap &);
    SharedPtr<Node> parse_unary_operator(LocalsHashmap &);
    SharedPtr<Node> parse_undef(LocalsHashmap &);
    SharedPtr<Node> parse_unless(LocalsHashmap &);
    SharedPtr<Node> parse_while(LocalsHashmap &);
    SharedPtr<Node> parse_word_array(LocalsHashmap &);
    SharedPtr<Node> parse_word_symbol_array(LocalsHashmap &);
    SharedPtr<Node> parse_yield(LocalsHashmap &);

    SharedPtr<Node> parse_assignment_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_assignment_expression_without_multiple_values(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_assignment_expression(SharedPtr<Node>, LocalsHashmap &, bool);
    SharedPtr<Node> parse_assignment_expression_value(bool, LocalsHashmap &, bool);
    SharedPtr<Node> parse_assignment_identifier(bool, LocalsHashmap &);
    SharedPtr<Node> parse_call_expression_without_parens(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_call_expression_with_parens(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_call_hash_args(LocalsHashmap &, bool, Token::Type, SharedPtr<Node>);
    SharedPtr<Node> parse_constant_resolution_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_infix_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_proc_call_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_iter_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<BlockNode> parse_iter_body(LocalsHashmap &, bool);
    SharedPtr<Node> parse_logical_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_match_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_modifier_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_multiple_assignment_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_not_match_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_op_assign_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_op_attr_assign_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_range_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_ref_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_rescue_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_safe_send_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_send_expression(SharedPtr<Node>, LocalsHashmap &);
    SharedPtr<Node> parse_ternary_expression(SharedPtr<Node>, LocalsHashmap &);

    void parse_call_args(NodeWithArgs &, LocalsHashmap &, bool = false, Token::Type = Token::Type::RParen);
    void parse_iter_args(Vector<SharedPtr<Node>> &, LocalsHashmap &);

    using parse_null_fn = SharedPtr<Node> (Parser::*)(LocalsHashmap &);
    using parse_left_fn = SharedPtr<Node> (Parser::*)(SharedPtr<Node>, LocalsHashmap &);

    parse_null_fn null_denotation(Token::Type);
    parse_left_fn left_denotation(Token &, SharedPtr<Node>, Precedence);

    bool treat_left_bracket_as_element_reference(SharedPtr<Node> left, Token &token) {
        return !token.whitespace_precedes() || (left->type() == Node::Type::Identifier && left.static_cast_as<IdentifierNode>()->is_lvar());
    }

    // convert ((x and y) and z) to (x and (y and z))
    template <typename T>
    SharedPtr<Node> regroup(Token &token, SharedPtr<Node> left, SharedPtr<Node> right) {
        auto left_node = left.static_cast_as<T>();
        return new T { left_node->token(), left_node->left(), new T { token, left_node->right(), right } };
    };

    SharedPtr<Node> append_string_nodes(SharedPtr<Node> string1, SharedPtr<Node> string2);
    SharedPtr<Node> concat_adjacent_strings(SharedPtr<Node> string, LocalsHashmap &locals, bool &strings_were_appended);

    SharedPtr<NodeWithArgs> to_node_with_args(SharedPtr<Node> node);

    Token &current_token() const;
    Token &peek_token() const;

    void next_expression();
    void skip_newlines();

    void expect(Token::Type, const char *);
    [[noreturn]] void throw_error(const Token &, const char *);
    [[noreturn]] void throw_unexpected(const Token &, const char *, const char * = nullptr);
    [[noreturn]] void throw_unexpected(const char *);
    [[noreturn]] void throw_unterminated_thing(Token, Token = {});

    void advance() { m_index++; }
    void rewind() { m_index--; }

    String code_line(size_t number);
    String current_line();

    void validate_current_token();

    SharedPtr<String> m_code;
    SharedPtr<String> m_file;
    size_t m_index { 0 };
    SharedPtr<Vector<Token>> m_tokens {};

    Vector<Precedence> m_precedence_stack {};
    Vector<unsigned int> m_call_depth {};
};
}
