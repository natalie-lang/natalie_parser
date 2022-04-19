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
    }

    ~Parser() {
        // SharedPtr ftw
    }

    using LocalsHashmap = TM::Hashmap<TM::String>;

    enum class Precedence;

    SharedPtr<Node> tree();

private:
    bool higher_precedence(Token &token, Node *left, Precedence current_precedence);

    Precedence get_precedence(Token &token, Node *left = nullptr);

    bool is_first_arg_of_call_without_parens(Node *, Token &);

    Node *parse_expression(Precedence, LocalsHashmap &);

    BlockNode *parse_body(LocalsHashmap &, Precedence, Token::Type = Token::Type::EndKeyword, bool = false);
    BlockNode *parse_body(LocalsHashmap &, Precedence, Vector<Token::Type> &, const char *);
    BlockNode *parse_case_in_body(LocalsHashmap &);
    BlockNode *parse_case_when_body(LocalsHashmap &);
    Node *parse_if_body(LocalsHashmap &);
    BlockNode *parse_def_body(LocalsHashmap &);

    Node *parse_alias(LocalsHashmap &);
    SymbolNode *parse_alias_arg(LocalsHashmap &, const char *, bool);
    Node *parse_array(LocalsHashmap &);
    Node *parse_back_ref(LocalsHashmap &);
    Node *parse_begin(LocalsHashmap &);
    void parse_rest_of_begin(BeginNode *, LocalsHashmap &);
    Node *parse_beginless_range(LocalsHashmap &);
    Node *parse_block_pass(LocalsHashmap &);
    Node *parse_bool(LocalsHashmap &);
    Node *parse_break(LocalsHashmap &);
    Node *parse_class(LocalsHashmap &);
    Node *parse_class_or_module_name(LocalsHashmap &);
    Node *parse_case(LocalsHashmap &);
    Node *parse_case_in_pattern(LocalsHashmap &);
    Node *parse_case_in_patterns(LocalsHashmap &);
    void parse_comma_separated_expressions(ArrayNode *, LocalsHashmap &);
    Node *parse_constant(LocalsHashmap &);
    Node *parse_def(LocalsHashmap &);
    Node *parse_defined(LocalsHashmap &);
    SharedPtr<Vector<Node *>> parse_def_args(LocalsHashmap &);
    Node *parse_def_single_arg(LocalsHashmap &);
    Node *parse_file_constant(LocalsHashmap &);
    Node *parse_group(LocalsHashmap &);
    Node *parse_hash(LocalsHashmap &);
    Node *parse_hash_inner(LocalsHashmap &, Precedence, Token::Type, Node * = nullptr);
    Node *parse_identifier(LocalsHashmap &);
    Node *parse_if(LocalsHashmap &);
    void parse_interpolated_body(LocalsHashmap &, InterpolatedNode *, Token::Type);
    Node *parse_interpolated_regexp(LocalsHashmap &);
    int parse_regexp_options(String &);
    Node *parse_interpolated_shell(LocalsHashmap &);
    Node *parse_interpolated_string(LocalsHashmap &);
    Node *parse_interpolated_symbol(LocalsHashmap &);
    Node *parse_lit(LocalsHashmap &);
    Node *parse_keyword_splat(LocalsHashmap &);
    SharedPtr<String> parse_method_name(LocalsHashmap &);
    Node *parse_module(LocalsHashmap &);
    Node *parse_next(LocalsHashmap &);
    Node *parse_nil(LocalsHashmap &);
    Node *parse_not(LocalsHashmap &);
    Node *parse_nth_ref(LocalsHashmap &);
    Node *parse_redo(LocalsHashmap &);
    Node *parse_retry(LocalsHashmap &);
    Node *parse_return(LocalsHashmap &);
    Node *parse_sclass(LocalsHashmap &);
    Node *parse_self(LocalsHashmap &);
    Node *parse_splat(LocalsHashmap &);
    Node *parse_stabby_proc(LocalsHashmap &);
    Node *parse_string(LocalsHashmap &);
    Node *parse_super(LocalsHashmap &);
    Node *parse_symbol(LocalsHashmap &);
    Node *parse_symbol_key(LocalsHashmap &);
    Node *parse_statement_keyword(LocalsHashmap &);
    Node *parse_top_level_constant(LocalsHashmap &);
    Node *parse_unary_operator(LocalsHashmap &);
    Node *parse_undef(LocalsHashmap &);
    Node *parse_unless(LocalsHashmap &);
    Node *parse_while(LocalsHashmap &);
    Node *parse_word_array(LocalsHashmap &);
    Node *parse_word_symbol_array(LocalsHashmap &);
    Node *parse_yield(LocalsHashmap &);

    void parse_assignment_expression(Node *&, LocalsHashmap &);
    void parse_assignment_expression_without_multiple_values(Node *&, LocalsHashmap &);
    void parse_assignment_expression(Node *&, LocalsHashmap &, bool);
    Node *parse_assignment_expression_value(bool, LocalsHashmap &, bool);
    Node *parse_assignment_identifier(bool, LocalsHashmap &);
    void parse_call_expression_without_parens(Node *&, LocalsHashmap &);
    void parse_call_expression_with_parens(Node *&, LocalsHashmap &);
    void parse_call_args(NodeWithArgs *, LocalsHashmap &, bool = false);
    Node *parse_call_hash_args(LocalsHashmap &, bool, Node *);
    void parse_constant_resolution_expression(Node *&, LocalsHashmap &);
    void parse_infix_expression(Node *&, LocalsHashmap &);
    void parse_proc_call_expression(Node *&, LocalsHashmap &);
    void parse_iter_expression(Node *&, LocalsHashmap &);
    void parse_iter_args(SharedPtr<Vector<Node *>>, LocalsHashmap &);
    BlockNode *parse_iter_body(LocalsHashmap &, bool);
    void parse_logical_expression(Node *&, LocalsHashmap &);
    void parse_match_expression(Node *&, LocalsHashmap &);
    void parse_modifier_expression(Node *&, LocalsHashmap &);
    void parse_multiple_assignment_expression(Node *&, LocalsHashmap &);
    void parse_not_match_expression(Node *&, LocalsHashmap &);
    void parse_op_assign_expression(Node *&, LocalsHashmap &);
    void parse_op_attr_assign_expression(Node *&, LocalsHashmap &);
    void parse_range_expression(Node *&, LocalsHashmap &);
    void parse_ref_expression(Node *&, LocalsHashmap &);
    void parse_rescue_expression(Node *&, LocalsHashmap &);
    void parse_safe_send_expression(Node *&, LocalsHashmap &);
    void parse_send_expression(Node *&, LocalsHashmap &);
    void parse_ternary_expression(Node *&, LocalsHashmap &);

    using parse_null_fn = Node *(Parser::*)(LocalsHashmap &);
    using parse_left_fn = void (Parser::*)(Node *&, LocalsHashmap &);

    parse_null_fn null_denotation(Token::Type);
    parse_left_fn left_denotation(Token &, Node *, Precedence);

    bool treat_left_bracket_as_element_reference(Node *left, Token &token) {
        return !token.whitespace_precedes() || (left->type() == Node::Type::Identifier && static_cast<IdentifierNode *>(left)->is_lvar());
    }

    // convert ((x and y) and z) to (x and (y and z))
    template <typename T>
    Node *regroup(Token &token, Node *left, Node *right) {
        auto left_node = static_cast<T *>(left);
        auto new_node = new T { left_node->token(), left_node->left().clone(), new T { token, left_node->right().clone(), right } };
        delete left_node;
        return new_node;
    };

    Node *append_string_nodes(Node *string1, Node *string2);
    Node *concat_adjacent_strings(Node *string, LocalsHashmap &locals, bool &strings_were_appended);

    NodeWithArgs *to_node_with_args(Node *&node);

    // FIXME: return a Token&
    Token current_token() const;
    Token peek_token() const;

    void next_expression();
    void skip_newlines();

    void expect(Token::Type, const char *);
    [[noreturn]] void throw_unexpected(const Token &, const char *, const char * = nullptr);
    [[noreturn]] void throw_unexpected(const char *);
    [[noreturn]] void throw_unterminated_thing(Token);

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
};
}
