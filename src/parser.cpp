#include "natalie_parser/parser.hpp"
#include "natalie_parser/creator/debug_creator.hpp"

namespace NatalieParser {

enum class Parser::Precedence {
    LOWEST,
    ARRAY, // []
    WORD_ARRAY, // %w[]
    HASH, // {}
    EXPR_MODIFIER, // if/unless/while/until
    CASE, // case/when/else
    COMPOSITION, // and/or
    TERNARY_FALSE, // _ ? _ : (_)
    ASSIGNMENT_RHS, // a = (_)
    ITER_BLOCK, // do |n| ... end
    BARE_CALL_ARG, // foo (_), b
    OP_ASSIGNMENT, // += -= *= **= /= %= |= &= ^= >>= <<= ||= &&=
    TERNARY_TRUE, // _ ? (_) : _
    CALL_ARG, // foo( (_), b )
    TERNARY_QUESTION, // (_) ? _ : _
    LOGICAL_OR, // ||
    LOGICAL_AND, // &&
    ASSIGNMENT_LHS, // (_) = 1
    SPLAT, // *args, **kwargs
    RANGE, // ..
    LOGICAL_NOT, // not
    EQUALITY, // <=> == === != =~ !~
    LESS_GREATER, // <= < > >=
    BITWISE_OR, // ^ |
    BITWISE_AND, // &
    BITWISE_SHIFT, // << >>
    DEF_ARG, // def foo( (_), b ) and { | (_), b | ... }
    SUM, // + -
    PRODUCT, // * / %
    NUMBER_DOT, // 2.bar 2&.bar
    UNARY_MINUS, // -
    EXPONENT, // **
    UNARY_PLUS, // ! ~ +
    ITER_CURLY, // { |n| ... }
    CONSTANT_RESOLUTION, // ::
    DOT, // foo.bar foo&.bar
    CALL, // foo()
    REF, // foo[1] / foo[1] = 2
};

bool Parser::higher_precedence(Token &token, Node *left, Precedence current_precedence) {
    auto next_precedence = get_precedence(token, left);

    // printf("token %d, left %d, current_precedence %d, next_precedence %d\n", (int)token.type(), (int)left->type(), (int)current_precedence, (int)next_precedence);

    if (left->is_symbol_key()) {
        // Symbol keys are handled by parse_hash and parse_call_hash_args,
        // so return as soon as possible to one of those functions.
        return false;
    }

    if (next_precedence == Precedence::ITER_BLOCK && next_precedence <= current_precedence) {
        // Simple precedence comparison to the nearest neighbor is not
        // sufficient when BARE_CALL_ARG (a method call without parentheses)
        // is in use. For example, the following code, if looking at
        // precedence rules alone, would attach the block to the '+' op,
        // which would be incorrect:
        //
        //     bar + baz do ... end
        //         ^ block should NOT attach here
        //
        // Rather, we want:
        //
        //     bar + baz do ... end
        //           ^ block attaches here
        //
        // But changing the precedence order cannot fix this, because in
        // many cases, we need the block to attach farther left, e.g.:
        //
        //     foo bar + baz do ... end
        //     ^ block attaches here
        //
        // Thus the answer is to keep a stack of precedences and look
        // further left to see if BARE_CALL_ARG are in use. If not, then
        // we should attach the block now, to the nearest call
        // (which is to say, we should return true).
        //
        for (auto precedence : m_precedence_stack) {
            if (precedence == Precedence::BARE_CALL_ARG)
                return false;
        }
        return true;
    }

    return next_precedence > current_precedence;
}

Parser::Precedence Parser::get_precedence(Token &token, Node *left) {
    switch (token.type()) {
    case Token::Type::Plus:
        return left ? Precedence::SUM : Precedence::UNARY_PLUS;
    case Token::Type::Minus:
        return left ? Precedence::SUM : Precedence::UNARY_MINUS;
    case Token::Type::Equal:
        return Precedence::ASSIGNMENT_LHS;
    case Token::Type::AndEqual:
    case Token::Type::BitwiseAndEqual:
    case Token::Type::BitwiseOrEqual:
    case Token::Type::BitwiseXorEqual:
    case Token::Type::DivideEqual:
    case Token::Type::ExponentEqual:
    case Token::Type::LeftShiftEqual:
    case Token::Type::MinusEqual:
    case Token::Type::ModulusEqual:
    case Token::Type::MultiplyEqual:
    case Token::Type::OrEqual:
    case Token::Type::PlusEqual:
    case Token::Type::RightShiftEqual:
        return Precedence::OP_ASSIGNMENT;
    case Token::Type::BitwiseAnd:
        return Precedence::BITWISE_AND;
    case Token::Type::BitwiseOr:
    case Token::Type::BitwiseXor:
        return Precedence::BITWISE_OR;
    case Token::Type::Comma:
        // NOTE: the only time this precedence is used is for multiple assignment
        return Precedence::ARRAY;
    case Token::Type::LeftShift:
    case Token::Type::RightShift:
        return Precedence::BITWISE_SHIFT;
    case Token::Type::LParen:
        return Precedence::CALL;
    case Token::Type::AndKeyword:
    case Token::Type::OrKeyword:
        return Precedence::COMPOSITION;
    case Token::Type::ConstantResolution:
        return Precedence::CONSTANT_RESOLUTION;
    case Token::Type::Dot:
    case Token::Type::SafeNavigation:
        if (left && left->is_numeric())
            return Precedence::NUMBER_DOT;
        return Precedence::DOT;
    case Token::Type::EqualEqual:
    case Token::Type::EqualEqualEqual:
    case Token::Type::NotEqual:
    case Token::Type::Match:
    case Token::Type::NotMatch:
        return Precedence::EQUALITY;
    case Token::Type::Exponent:
        return Precedence::EXPONENT;
    case Token::Type::IfKeyword:
    case Token::Type::UnlessKeyword:
    case Token::Type::WhileKeyword:
    case Token::Type::UntilKeyword:
    case Token::Type::RescueKeyword:
        return Precedence::EXPR_MODIFIER;
    case Token::Type::DoKeyword:
        return Precedence::ITER_BLOCK;
    case Token::Type::LCurlyBrace:
        return Precedence::ITER_CURLY;
    case Token::Type::Comparison:
    case Token::Type::LessThan:
    case Token::Type::LessThanOrEqual:
    case Token::Type::GreaterThan:
    case Token::Type::GreaterThanOrEqual:
        return Precedence::LESS_GREATER;
    case Token::Type::And:
        return Precedence::LOGICAL_AND;
    case Token::Type::NotKeyword:
        return Precedence::LOGICAL_NOT;
    case Token::Type::Or:
        return Precedence::LOGICAL_OR;
    case Token::Type::Divide:
    case Token::Type::Modulus:
    case Token::Type::Multiply:
        return Precedence::PRODUCT;
    case Token::Type::DotDot:
    case Token::Type::DotDotDot:
        return Precedence::RANGE;
    case Token::Type::LBracket:
    case Token::Type::LBracketRBracket: {
        auto current = current_token();
        if (left && treat_left_bracket_as_element_reference(left, current))
            return Precedence::REF;
        break;
    }
    case Token::Type::TernaryQuestion:
        return Precedence::TERNARY_QUESTION;
    case Token::Type::TernaryColon:
        return Precedence::TERNARY_FALSE;
    case Token::Type::Not:
    case Token::Type::Tilde:
        return Precedence::UNARY_PLUS;
    default:
        break;
    }
    auto current = current_token();
    if (left && is_first_arg_of_call_without_parens(left, current))
        return Precedence::CALL;
    return Precedence::LOWEST;
}

Node *Parser::parse_expression(Parser::Precedence precedence, LocalsHashmap &locals) {
    skip_newlines();

    m_precedence_stack.push(precedence);

    auto null_fn = null_denotation(current_token().type());
    if (!null_fn)
        throw_unexpected("expression");

    Node *left = (this->*null_fn)(locals);

    while (current_token().is_valid()) {
        auto token = current_token();
        if (!higher_precedence(token, left, precedence))
            break;
        auto left_fn = left_denotation(token, left, precedence);
        if (!left_fn) {
            printf("left_denotation returned nullptr (token type = %d, node type = %d, precedence = %d)\n", (int)token.type(), (int)left->type(), (int)precedence);
            TM_UNREACHABLE();
        }
        left = (this->*left_fn)(left, locals);
        m_precedence_stack.pop();
        m_precedence_stack.push(precedence);
    }

    m_precedence_stack.pop();

    return left;
}

Node *Parser::tree() {
    auto tree = new BlockNode { current_token() };
    current_token().validate();
    LocalsHashmap locals { TM::HashType::TMString };
    skip_newlines();
    while (!current_token().is_eof()) {
        auto exp = parse_expression(Precedence::LOWEST, locals);
        tree->add_node(exp);
        current_token().validate();
        next_expression();
    }
    return tree;
}

BlockNode *Parser::parse_body(LocalsHashmap &locals, Precedence precedence, Token::Type end_token_type, bool allow_rescue) {
    auto body = new BlockNode { current_token() };
    current_token().validate();
    skip_newlines();
    while (!current_token().is_eof() && current_token().type() != end_token_type) {
        if (allow_rescue && current_token().type() == Token::Type::RescueKeyword) {
            auto begin_node = new BeginNode { body->token(), body };
            parse_rest_of_begin(begin_node, locals);
            rewind(); // so the 'end' keyword can be consumed by our caller
            return new BlockNode { body->token(), begin_node };
        }
        auto exp = parse_expression(precedence, locals);
        body->add_node(exp);
        next_expression();
    }
    return body;
}

// FIXME: Maybe pass a lambda in that can just compare the two types? (No vector needed.)
BlockNode *Parser::parse_body(LocalsHashmap &locals, Precedence precedence, Vector<Token::Type> &end_tokens, const char *expected_message) {
    auto body = new BlockNode { current_token() };
    current_token().validate();
    skip_newlines();
    auto finished = [this, end_tokens] {
        for (auto end_token : end_tokens) {
            if (current_token().type() == end_token)
                return true;
        }
        return false;
    };
    while (!current_token().is_eof() && !finished()) {
        auto exp = parse_expression(precedence, locals);
        body->add_node(exp);
        current_token().validate();
        next_expression();
    }
    if (!finished())
        throw_unexpected(expected_message);
    return body;
}

BlockNode *Parser::parse_def_body(LocalsHashmap &locals) {
    auto token = current_token();
    auto body = new BlockNode { token };
    skip_newlines();
    while (!current_token().is_eof() && !current_token().is_end_keyword()) {
        if (current_token().type() == Token::Type::RescueKeyword) {
            auto begin_node = new BeginNode { token, body };
            parse_rest_of_begin(begin_node, locals);
            rewind(); // so the 'end' keyword can be consumed by parse_def
            return new BlockNode { token, begin_node };
        }
        auto exp = parse_expression(Precedence::LOWEST, locals);
        body->add_node(exp);
        next_expression();
    }
    return body;
}

Node *Parser::parse_alias(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto new_name = parse_alias_arg(locals, "alias new name (first argument)", false);
    auto existing_name = parse_alias_arg(locals, "alias existing name (second argument)", true);
    return new AliasNode { token, new_name, existing_name };
}

SymbolNode *Parser::parse_alias_arg(LocalsHashmap &locals, const char *expected_message, bool reinsert_collapsed_newline) {
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::BareName: {
        auto identifier = static_cast<IdentifierNode *>(parse_identifier(locals));
        return new SymbolNode { token, identifier->name() };
    }
    case Token::Type::Symbol:
        return static_cast<SymbolNode *>(parse_symbol(locals));
    case Token::Type::InterpolatedSymbolBegin:
        return static_cast<SymbolNode *>(parse_interpolated_symbol(locals));
    default:
        if (token.is_operator() || token.is_keyword()) {
            advance();
            if (token.can_precede_collapsible_newline() && reinsert_collapsed_newline) {
                // Some operators at the end of a line cause the newlines to be collapsed:
                //
                //     foo <<
                //       bar
                //
                // ...but in this case (an alias), collapsing the newline was a mistake:
                //
                //     alias foo <<
                //     def bar; end
                //
                // So, we'll put the newline back.
                m_tokens->insert(m_index, Token { Token::Type::Eol, token.file(), token.line(), token.column() });
            }
            return new SymbolNode { token, new String(token.type_value()) };
        } else {
            throw_unexpected(expected_message);
        }
    }
}

Node *Parser::parse_array(LocalsHashmap &locals) {
    auto array = new ArrayNode { current_token() };
    if (current_token().type() == Token::Type::LBracketRBracket) {
        advance();
        return array;
    }
    advance();
    auto add_node = [&]() -> Node * {
        auto token = current_token();
        if (token.type() == Token::Type::RBracket) {
            advance();
            return array;
        }
        if (token.type() == Token::Type::SymbolKey) {
            array->add_node(parse_hash_inner(locals, Precedence::HASH, Token::Type::RBracket));
            expect(Token::Type::RBracket, "array closing bracket");
            advance();
            return array;
        }
        auto value = parse_expression(Precedence::ARRAY, locals);
        token = current_token();
        if (token.is_hash_rocket()) {
            array->add_node(parse_hash_inner(locals, Precedence::HASH, Token::Type::RBracket, value));
            expect(Token::Type::RBracket, "array closing bracket");
            advance();
            return array;
        }
        array->add_node(value);
        return nullptr;
    };
    auto ret = add_node();
    if (ret) return ret;
    while (current_token().is_comma()) {
        advance();
        ret = add_node();
        if (ret) return ret;
    }
    expect(Token::Type::RBracket, "array closing bracket");
    advance();
    return array;
}

void Parser::parse_comma_separated_expressions(ArrayNode *array, LocalsHashmap &locals) {
    array->add_node(parse_expression(Precedence::ARRAY, locals));
    while (current_token().type() == Token::Type::Comma) {
        advance();
        array->add_node(parse_expression(Precedence::ARRAY, locals));
    }
}

Node *Parser::parse_back_ref(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new BackRefNode { token, token.literal_string()->at(0) };
}

Node *Parser::parse_begin(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    next_expression();
    auto begin_ending_tokens = Vector<Token::Type> { { Token::Type::RescueKeyword, Token::Type::ElseKeyword, Token::Type::EnsureKeyword, Token::Type::EndKeyword } };
    auto body = parse_body(locals, Precedence::LOWEST, begin_ending_tokens, "case: rescue, else, ensure, or end");

    auto begin_node = new BeginNode { token, body };
    parse_rest_of_begin(begin_node, locals);

    // a begin/end with nothing else just becomes a BlockNode
    if (!(begin_node->has_rescue_nodes() || begin_node->has_else_body() || begin_node->has_ensure_body())) {
        auto block_node = new BlockNode { begin_node->body() };
        delete begin_node;
        return block_node;
    }

    return begin_node;
}

void Parser::parse_rest_of_begin(BeginNode *begin_node, LocalsHashmap &locals) {
    auto rescue_ending_tokens = Vector<Token::Type> { { Token::Type::RescueKeyword, Token::Type::ElseKeyword, Token::Type::EnsureKeyword, Token::Type::EndKeyword } };
    auto else_ending_tokens = Vector<Token::Type> { { Token::Type::EnsureKeyword, Token::Type::EndKeyword } };
    while (!current_token().is_eof() && !current_token().is_end_keyword()) {
        switch (current_token().type()) {
        case Token::Type::RescueKeyword: {
            auto rescue_node = new BeginRescueNode { current_token() };
            advance();
            if (!current_token().is_eol() && current_token().type() != Token::Type::HashRocket) {
                auto name = parse_expression(Precedence::BARE_CALL_ARG, locals);
                rescue_node->add_exception_node(name);
                while (current_token().is_comma()) {
                    advance();
                    auto name = parse_expression(Precedence::BARE_CALL_ARG, locals);
                    rescue_node->add_exception_node(name);
                }
            }
            if (current_token().type() == Token::Type::HashRocket) {
                advance();
                auto name = static_cast<IdentifierNode *>(parse_identifier(locals));
                name->add_to_locals(locals);
                rescue_node->set_exception_name(name);
            }
            next_expression();
            auto body = parse_body(locals, Precedence::LOWEST, rescue_ending_tokens, "case: rescue, else, ensure, or end");
            rescue_node->set_body(body);
            begin_node->add_rescue_node(rescue_node);
            break;
        }
        case Token::Type::ElseKeyword: {
            advance();
            next_expression();
            auto body = parse_body(locals, Precedence::LOWEST, else_ending_tokens, "case: ensure or end");
            begin_node->set_else_body(body);
            break;
        }
        case Token::Type::EnsureKeyword: {
            advance();
            next_expression();
            auto body = parse_body(locals, Precedence::LOWEST);
            begin_node->set_ensure_body(body);
            break;
        }
        default:
            throw_unexpected("begin end");
        }
    }
    expect(Token::Type::EndKeyword, "begin/rescue/ensure end");
    advance();
}

Node *Parser::parse_beginless_range(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    return new RangeNode {
        token,
        new NilNode { token },
        parse_expression(Precedence::LOWEST, locals),
        token.type() == Token::Type::DotDotDot
    };
}

Node *Parser::parse_block_pass(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto value = parse_expression(Precedence::UNARY_PLUS, locals);
    return new BlockPassNode { token, value };
}

Node *Parser::parse_bool(LocalsHashmap &) {
    auto token = current_token();
    switch (current_token().type()) {
    case Token::Type::TrueKeyword:
        advance();
        return new TrueNode { token };
    case Token::Type::FalseKeyword:
        advance();
        return new FalseNode { token };
    default:
        TM_UNREACHABLE();
    }
}

Node *Parser::parse_break(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    if (current_token().is_lparen()) {
        advance();
        if (current_token().is_rparen()) {
            advance();
            return new BreakNode { token, new NilSexpNode { token } };
        } else {
            auto arg = parse_expression(Precedence::BARE_CALL_ARG, locals);
            expect(Token::Type::RParen, "break closing paren");
            advance();
            return new BreakNode { token, arg };
        }
    } else if (current_token().can_be_first_arg_of_implicit_call()) {
        auto value = parse_expression(Precedence::BARE_CALL_ARG, locals);
        if (current_token().is_comma()) {
            auto array = new ArrayNode { token };
            array->add_node(value);
            while (current_token().is_comma()) {
                advance();
                array->add_node(parse_expression(Precedence::BARE_CALL_ARG, locals));
            }
            value = array;
        }
        return new BreakNode { token, value };
    }
    return new BreakNode { token };
}

Node *Parser::parse_case(LocalsHashmap &locals) {
    auto case_token = current_token();
    advance();
    Node *subject;
    if (current_token().type() == Token::Type::WhenKeyword) {
        subject = new NilNode { case_token };
    } else {
        subject = parse_expression(Precedence::CASE, locals);
        next_expression();
    }
    auto node = new CaseNode { case_token, subject };
    while (!current_token().is_end_keyword()) {
        auto token = current_token();
        switch (token.type()) {
        case Token::Type::WhenKeyword: {
            advance();
            auto condition_array = new ArrayNode { token };
            parse_comma_separated_expressions(condition_array, locals);
            if (current_token().type() == Token::Type::ThenKeyword) {
                advance();
                skip_newlines();
            } else {
                next_expression();
            }
            auto body = parse_case_when_body(locals);
            auto when_node = new CaseWhenNode { token, condition_array, body };
            node->add_node(when_node);
            break;
        }
        case Token::Type::InKeyword: {
            advance();
            auto pattern = parse_case_in_patterns(locals);
            if (current_token().type() == Token::Type::ThenKeyword) {
                advance();
                skip_newlines();
            } else {
                next_expression();
            }
            auto body = parse_case_in_body(locals);
            auto in_node = new CaseInNode { token, pattern, body };
            node->add_node(in_node);
            break;
        }
        case Token::Type::ElseKeyword: {
            if (node->nodes().is_empty())
                throw_unexpected("case 'when' or 'in'");
            advance();
            skip_newlines();
            BlockNode *body = parse_body(locals, Precedence::LOWEST);
            node->set_else_node(body);
            expect(Token::Type::EndKeyword, "case end");
            break;
        }
        default:
            throw_unexpected("case when keyword");
        }
    }
    expect(Token::Type::EndKeyword, "case end");
    advance();
    return node;
}

BlockNode *Parser::parse_case_in_body(LocalsHashmap &locals) {
    return parse_case_when_body(locals);
}

Node *Parser::parse_case_in_pattern(LocalsHashmap &locals) {
    auto token = current_token();
    Node *node;
    switch (token.type()) {
    case Token::Type::BareName:
        advance();
        node = new IdentifierNode { token, true };
        break;
    case Token::Type::BitwiseXor: // caret (^)
        advance();
        expect(Token::Type::BareName, "pinned variable name");
        node = new PinNode { token, new IdentifierNode { current_token(), true } };
        advance();
        break;
    case Token::Type::LBracketRBracket:
        advance();
        node = new ArrayPatternNode { token };
        break;
    case Token::Type::LBracket: {
        // TODO: might need to keep track of and pass along precedence value?
        advance();
        auto array = new ArrayPatternNode { token };
        if (current_token().type() == Token::Type::RBracket) {
            advance();
            node = array;
            break;
        }
        array->add_node(parse_case_in_pattern(locals));
        while (current_token().is_comma()) {
            advance();
            array->add_node(parse_case_in_pattern(locals));
        }
        expect(Token::Type::RBracket, "array pattern closing bracket");
        advance();
        node = array;
        break;
    }
    case Token::Type::LCurlyBrace: {
        advance();
        auto hash = new HashPatternNode { token };
        if (current_token().type() == Token::Type::RCurlyBrace) {
            advance();
            node = hash;
            break;
        }
        expect(Token::Type::SymbolKey, "hash pattern symbol key");
        hash->add_node(parse_symbol(locals));
        hash->add_node(parse_case_in_pattern(locals));
        while (current_token().is_comma()) {
            advance();
            hash->add_node(parse_symbol(locals));
            hash->add_node(parse_case_in_pattern(locals));
        }
        expect(Token::Type::RCurlyBrace, "hash pattern closing brace");
        advance();
        node = hash;
        break;
    }
    case Token::Type::Bignum:
    case Token::Type::Fixnum:
    case Token::Type::Float:
        node = parse_lit(locals);
        break;
    case Token::Type::String:
        node = parse_string(locals);
        break;
    case Token::Type::Symbol:
        node = parse_symbol(locals);
        break;
    default:
        printf("TODO: implement token type %d in Parser::parse_case_in_pattern()\n", (int)token.type());
        abort();
    }
    token = current_token();
    if (token.is_hash_rocket()) {
        advance();
        expect(Token::Type::BareName, "pattern name");
        token = current_token();
        advance();
        auto identifier = new IdentifierNode { token, true };
        node = new AssignmentNode { token, identifier, node };
    }
    return node;
}

Node *Parser::parse_case_in_patterns(LocalsHashmap &locals) {
    Vector<Node *> patterns;
    patterns.push(parse_case_in_pattern(locals));
    while (current_token().type() == Token::Type::BitwiseOr) {
        advance();
        patterns.push(parse_case_in_pattern(locals));
    }
    assert(patterns.size() > 0);
    if (patterns.size() == 1) {
        return patterns.first();
    } else {
        auto first = patterns.pop_front();
        auto second = patterns.pop_front();
        auto pattern = new LogicalOrNode { first->token(), first, second };
        while (!patterns.is_empty()) {
            auto next = patterns.pop_front();
            pattern = new LogicalOrNode { next->token(), pattern, next };
        }
        return pattern;
    }
}

BlockNode *Parser::parse_case_when_body(LocalsHashmap &locals) {
    auto body = new BlockNode { current_token() };
    current_token().validate();
    skip_newlines();
    while (!current_token().is_eof() && !current_token().is_when_keyword() && !current_token().is_else_keyword() && !current_token().is_end_keyword()) {
        auto exp = parse_expression(Precedence::LOWEST, locals);
        body->add_node(exp);
        current_token().validate();
        next_expression();
    }
    if (!current_token().is_when_keyword() && !current_token().is_else_keyword() && !current_token().is_end_keyword())
        throw_unexpected("case: when, else, or end");
    return body;
}

Node *Parser::parse_class_or_module_name(LocalsHashmap &locals) {
    Token name_token;
    if (current_token().type() == Token::Type::ConstantResolution) {
        name_token = peek_token();
    } else {
        name_token = current_token();
    }
    if (name_token.type() != Token::Type::Constant)
        throw SyntaxError { "class/module name must be CONSTANT" };
    return parse_expression(Precedence::LESS_GREATER, locals);
}

Node *Parser::parse_class(LocalsHashmap &locals) {
    auto token = current_token();
    if (peek_token().type() == Token::Type::LeftShift)
        return parse_sclass(locals);
    advance();
    LocalsHashmap our_locals { TM::HashType::TMString };
    auto name = parse_class_or_module_name(our_locals);
    Node *superclass;
    if (current_token().type() == Token::Type::LessThan) {
        advance();
        superclass = parse_expression(Precedence::LOWEST, our_locals);
    } else {
        superclass = new NilNode { token };
    }
    auto body = parse_body(our_locals, Precedence::LOWEST, Token::Type::EndKeyword, true);
    expect(Token::Type::EndKeyword, "class end");
    advance();
    return new ClassNode { token, name, superclass, body };
};

Node *Parser::parse_multiple_assignment_expression(Node *left, LocalsHashmap &locals) {
    auto list = new MultipleAssignmentNode { left->token() };
    list->add_node(left);
    while (current_token().is_comma()) {
        advance();
        switch (current_token().type()) {
        case Token::Type::BareName:
        case Token::Type::ClassVariable:
        case Token::Type::Constant:
        case Token::Type::ConstantResolution:
        case Token::Type::GlobalVariable:
        case Token::Type::InstanceVariable:
            list->add_node(parse_expression(Precedence::ASSIGNMENT_LHS, locals));
            break;
        case Token::Type::LParen:
            list->add_node(parse_group(locals));
            break;
        case Token::Type::Multiply: {
            auto splat_token = current_token();
            advance();
            if (current_token().is_assignable()) {
                auto node = parse_expression(Precedence::ASSIGNMENT_LHS, locals);
                list->add_node(new SplatNode { splat_token, node });
            } else {
                list->add_node(new SplatNode { splat_token });
            }
            break;
        }
        default:
            expect(Token::Type::BareName, "assignment identifier");
        }
    }
    return list;
}

Node *Parser::parse_constant(LocalsHashmap &) {
    auto node = new ConstantNode { current_token() };
    advance();
    return node;
};

Node *Parser::parse_def(LocalsHashmap &locals) {
    auto def_token = current_token();
    advance();
    LocalsHashmap our_locals { TM::HashType::TMString };
    Node *self_node = nullptr;
    SharedPtr<String> name = new String("");
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::BareName:
        if (peek_token().type() == Token::Type::Dot) {
            self_node = parse_identifier(locals);
            advance(); // dot
        }
        name = parse_method_name(locals);
        break;
    case Token::Type::Constant:
        if (peek_token().type() == Token::Type::Dot) {
            self_node = parse_constant(locals);
            advance(); // dot
        }
        name = parse_method_name(locals);
        break;
    case Token::Type::SelfKeyword:
        if (peek_token().type() == Token::Type::Dot) {
            self_node = new SelfNode { current_token() };
            advance(); // self
            advance(); // dot
        }
        name = parse_method_name(locals);
        break;
    default: {
        if (token.is_operator() || token.is_keyword()) {
            name = parse_method_name(locals);
        } else {
            self_node = parse_expression(Precedence::DOT, locals);
            expect(Token::Type::Dot, "dot followed by method name");
            advance(); // dot
            name = parse_method_name(locals);
        }
    }
    }
    if (current_token().type() == Token::Type::Equal && !current_token().whitespace_precedes()) {
        advance();
        name->append_char('=');
    }
    SharedPtr<Vector<Node *>> args = new Vector<Node *> {};
    if (current_token().is_lparen()) {
        advance();
        if (current_token().is_rparen()) {
            advance();
        } else {
            args = parse_def_args(our_locals);
            expect(Token::Type::RParen, "args closing paren");
            advance();
        }
    } else if (current_token().is_bare_name() || current_token().is_splat()) {
        args = parse_def_args(our_locals);
    }
    auto body = parse_def_body(our_locals);
    expect(Token::Type::EndKeyword, "def end");
    advance();
    return new DefNode { def_token, self_node, name, *args, body };
};

Node *Parser::parse_defined(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto arg = parse_expression(Precedence::BARE_CALL_ARG, locals);
    return new DefinedNode { token, arg };
}

SharedPtr<Vector<Node *>> Parser::parse_def_args(LocalsHashmap &locals) {
    SharedPtr<Vector<Node *>> args = new Vector<Node *> {};
    args->push(parse_def_single_arg(locals));
    while (current_token().is_comma()) {
        advance();
        args->push(parse_def_single_arg(locals));
    }
    return args;
}

Node *Parser::parse_def_single_arg(LocalsHashmap &locals) {
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::BareName: {
        auto arg = new ArgNode { token, token.literal_string() };
        advance();
        arg->add_to_locals(locals);
        if (current_token().type() == Token::Type::Equal) {
            advance();
            arg->set_value(parse_expression(Precedence::DEF_ARG, locals));
        }
        return arg;
    }
    case Token::Type::LParen: {
        advance();
        auto sub_args = parse_def_args(locals);
        expect(Token::Type::RParen, "nested args closing paren");
        advance();
        auto masgn = new MultipleAssignmentArgNode { token };
        for (auto arg : *sub_args) {
            masgn->add_node(arg);
        }
        return masgn;
    }
    case Token::Type::Multiply: {
        advance();
        ArgNode *arg;
        if (current_token().is_bare_name()) {
            arg = new ArgNode { token, current_token().literal_string() };
            advance();
            arg->add_to_locals(locals);
        } else {
            arg = new ArgNode { token };
        }
        arg->set_splat(true);
        return arg;
    }
    case Token::Type::Exponent: {
        advance();
        ArgNode *arg;
        if (current_token().is_bare_name()) {
            arg = new ArgNode { token, current_token().literal_string() };
            advance();
            arg->add_to_locals(locals);
        } else {
            arg = new ArgNode { token };
        }
        arg->set_kwsplat(true);
        return arg;
    }
    case Token::Type::BitwiseAnd: {
        advance();
        expect(Token::Type::BareName, "block name");
        auto arg = new ArgNode { token, current_token().literal_string() };
        advance();
        arg->add_to_locals(locals);
        arg->set_block_arg(true);
        return arg;
    }
    case Token::Type::SymbolKey: {
        auto arg = new KeywordArgNode { token, current_token().literal_string() };
        advance();
        switch (current_token().type()) {
        case Token::Type::Comma:
        case Token::Type::RParen:
        case Token::Type::Eol:
        case Token::Type::BitwiseOr:
            break;
        default:
            arg->set_value(parse_expression(Precedence::DEF_ARG, locals));
        }
        arg->add_to_locals(locals);
        return arg;
    }
    default:
        throw_unexpected("argument");
    }
}

Node *Parser::parse_modifier_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::IfKeyword: {
        advance();
        auto condition = parse_expression(Precedence::LOWEST, locals);
        return new IfNode { token, condition, left, new NilNode { token } };
    }
    case Token::Type::UnlessKeyword: {
        advance();
        auto condition = parse_expression(Precedence::LOWEST, locals);
        return new IfNode { token, condition, new NilNode { token }, left };
    }
    case Token::Type::UntilKeyword: {
        advance();
        auto condition = parse_expression(Precedence::LOWEST, locals);
        BlockNode *body;
        bool pre = true;
        if (left->type() == Node::Type::Block) {
            body = static_cast<BlockNode *>(left);
            pre = false;
        } else {
            if (left->type() == Node::Type::Begin) pre = false;
            body = new BlockNode { token, left };
        }
        return new UntilNode { token, condition, body, pre };
    }
    case Token::Type::WhileKeyword: {
        advance();
        auto condition = parse_expression(Precedence::LOWEST, locals);
        BlockNode *body;
        bool pre = true;
        if (left->type() == Node::Type::Block) {
            body = static_cast<BlockNode *>(left);
            pre = false;
        } else {
            if (left->type() == Node::Type::Begin) pre = false;
            body = new BlockNode { token, left };
        }
        return new WhileNode { token, condition, body, pre };
    }
    default:
        TM_NOT_YET_IMPLEMENTED();
    }
}

Node *Parser::parse_file_constant(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new StringNode { token, token.file() };
}

Node *Parser::parse_group(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    if (current_token().is_rparen()) {
        advance();
        return new NilSexpNode { token };
    }
    auto exp = parse_expression(Precedence::LOWEST, locals);
    if (current_token().is_end_of_expression()) {
        auto block = new BlockNode { token };
        block->add_node(exp);
        while (current_token().is_end_of_expression()) {
            next_expression();
            auto next_exp = parse_expression(Precedence::LOWEST, locals);
            block->add_node(next_exp);
        }
        exp = block;
    }
    expect(Token::Type::RParen, "group closing paren");
    advance();
    return exp;
};

Node *Parser::parse_hash(LocalsHashmap &locals) {
    expect(Token::Type::LCurlyBrace, "hash opening curly brace");
    auto token = current_token();
    advance();
    Node *hash;
    if (current_token().type() == Token::Type::RCurlyBrace)
        hash = new HashNode { token };
    else
        hash = parse_hash_inner(locals, Precedence::HASH, Token::Type::RCurlyBrace);
    expect(Token::Type::RCurlyBrace, "hash closing curly brace");
    advance();
    return hash;
}

Node *Parser::parse_hash_inner(LocalsHashmap &locals, Precedence precedence, Token::Type closing_token, Node *first_key) {
    auto token = current_token();
    auto hash = new HashNode { token };
    if (!first_key)
        first_key = parse_expression(precedence, locals);
    hash->add_node(first_key);
    if (!first_key->is_symbol_key()) {
        expect(Token::Type::HashRocket, "hash rocket");
        advance();
    }
    hash->add_node(parse_expression(precedence, locals));
    while (current_token().type() == Token::Type::Comma) {
        advance();
        if (current_token().type() == closing_token)
            break;
        auto key = parse_expression(precedence, locals);
        hash->add_node(key);
        if (!key->is_symbol_key()) {
            expect(Token::Type::HashRocket, "hash rocket");
            advance();
        }
        hash->add_node(parse_expression(precedence, locals));
    }
    return hash;
}

Node *Parser::parse_identifier(LocalsHashmap &locals) {
    auto name = current_token().literal();
    bool is_lvar = !!locals.get(name);
    auto identifier = new IdentifierNode { current_token(), is_lvar };
    advance();
    return identifier;
};

Node *Parser::parse_if(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto condition = parse_expression(Precedence::LOWEST, locals);
    next_expression();
    auto true_expr = parse_if_body(locals);
    Node *false_expr;
    if (current_token().is_elsif_keyword()) {
        false_expr = parse_if(locals);
        return new IfNode { current_token(), condition, true_expr, false_expr };
    } else {
        if (current_token().is_else_keyword()) {
            advance();
            false_expr = parse_if_body(locals);
        } else {
            false_expr = new NilNode { current_token() };
        }
        expect(Token::Type::EndKeyword, "if end");
        advance();
        return new IfNode { token, condition, true_expr, false_expr };
    }
}

Node *Parser::parse_if_body(LocalsHashmap &locals) {
    auto body = new BlockNode { current_token() };
    current_token().validate();
    skip_newlines();
    while (!current_token().is_eof() && !current_token().is_elsif_keyword() && !current_token().is_else_keyword() && !current_token().is_end_keyword()) {
        auto exp = parse_expression(Precedence::LOWEST, locals);
        body->add_node(exp);
        current_token().validate();
        next_expression();
    }
    if (!current_token().is_elsif_keyword() && !current_token().is_else_keyword() && !current_token().is_end_keyword()) {
        delete body;
        throw_unexpected("if end");
    }
    if (body->has_one_node()) {
        auto node = body->take_first_node();
        delete body;
        return node;
    } else {
        return body;
    }
}

void Parser::parse_interpolated_body(LocalsHashmap &locals, InterpolatedNode *node, Token::Type end_token) {
    while (current_token().is_valid() && current_token().type() != end_token) {
        switch (current_token().type()) {
        case Token::Type::EvaluateToStringBegin: {
            advance();
            auto block = new BlockNode { current_token() };
            while (current_token().type() != Token::Type::EvaluateToStringEnd) {
                block->add_node(parse_expression(Precedence::LOWEST, locals));
                skip_newlines();
            }
            advance();
            if (block->has_one_node()) {
                auto first = block->take_first_node();
                if (first->type() == Node::Type::String)
                    node->add_node(first);
                else
                    node->add_node(new EvaluateToStringNode { current_token(), first });
                delete block;
            } else {
                node->add_node(new EvaluateToStringNode { current_token(), block });
            }
            break;
        }
        case Token::Type::String:
            node->add_node(new StringNode { current_token(), current_token().literal_string() });
            advance();
            break;
        default:
            TM_UNREACHABLE();
        }
    }
    if (current_token().type() != end_token)
        TM_UNREACHABLE() // this shouldn't happen -- if it does, there is a bug in the Lexer
};

Node *Parser::parse_interpolated_regexp(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    if (current_token().type() == Token::Type::InterpolatedRegexpEnd) {
        auto regexp_node = new RegexpNode { token, new String };
        if (current_token().has_literal()) {
            auto str = current_token().literal_string().ref();
            int options_int = parse_regexp_options(str);
            regexp_node->set_options(options_int);
        }
        advance();
        return regexp_node;
    } else if (current_token().type() == Token::Type::String && peek_token().type() == Token::Type::InterpolatedRegexpEnd) {
        auto regexp_node = new RegexpNode { token, current_token().literal_string() };
        advance();
        if (current_token().has_literal()) {
            auto str = current_token().literal_string().ref();
            int options_int = parse_regexp_options(str);
            regexp_node->set_options(options_int);
        }
        advance();
        return regexp_node;
    } else {
        auto interpolated_regexp = new InterpolatedRegexpNode { token };
        parse_interpolated_body(locals, interpolated_regexp, Token::Type::InterpolatedRegexpEnd);
        if (current_token().has_literal()) {
            auto str = current_token().literal_string().ref();
            int options_int = parse_regexp_options(str);
            interpolated_regexp->set_options(options_int);
        }
        advance();
        return interpolated_regexp;
    }
};

int Parser::parse_regexp_options(String &options_string) {
    int options = 0;
    for (size_t i = 0; i < options_string.size(); ++i) {
        switch (options_string.at(i)) {
        case 'i':
            options |= 1;
            break;
        case 'x':
            options |= 2;
            break;
        case 'm':
            options |= 4;
            break;
        case 'e':
        case 's':
        case 'u':
            options |= 16;
            break;
        case 'n':
            options |= 32;
            break;
        default:
            break;
        }
    }
    return options;
}

Node *Parser::parse_interpolated_shell(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    if (current_token().type() == Token::Type::InterpolatedShellEnd) {
        auto shell = new ShellNode { token, new String("") };
        advance();
        return shell;
    } else if (current_token().type() == Token::Type::String && peek_token().type() == Token::Type::InterpolatedShellEnd) {
        auto shell = new ShellNode { token, current_token().literal_string() };
        advance();
        advance();
        return shell;
    } else {
        auto interpolated_shell = new InterpolatedShellNode { token };
        parse_interpolated_body(locals, interpolated_shell, Token::Type::InterpolatedShellEnd);
        advance();
        return interpolated_shell;
    }
};

static Node *convert_string_to_symbol_key(Node *string) {
    switch (string->type()) {
    case Node::Type::String: {
        auto token = string->token();
        auto name = static_cast<StringNode *>(string)->string();
        delete string;
        return new SymbolKeyNode { token, name };
    }
    case Node::Type::InterpolatedString: {
        auto token = string->token();
        auto node = new InterpolatedSymbolKeyNode { *static_cast<InterpolatedStringNode *>(string) };
        delete string;
        return node;
    }
    default:
        TM_UNREACHABLE();
    }
}

Node *Parser::parse_interpolated_string(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    Node *string;
    if (current_token().type() == Token::Type::InterpolatedStringEnd) {
        string = new StringNode { token, new String };
        advance();
    } else if (current_token().type() == Token::Type::String && peek_token().type() == Token::Type::InterpolatedStringEnd) {
        string = new StringNode { token, current_token().literal_string() };
        advance();
        advance();
    } else {
        auto interpolated_string = new InterpolatedStringNode { token };
        parse_interpolated_body(locals, interpolated_string, Token::Type::InterpolatedStringEnd);
        advance();
        string = interpolated_string;
    }

    bool adjacent_strings_were_appended = false;
    if (m_precedence_stack.is_empty() || m_precedence_stack.last() != Precedence::WORD_ARRAY)
        string = concat_adjacent_strings(string, locals, adjacent_strings_were_appended);

    if (!adjacent_strings_were_appended && current_token().type() == Token::Type::TernaryColon && !current_token().whitespace_precedes()) {
        advance();
        return convert_string_to_symbol_key(string);
    }

    return string;
};

Node *Parser::parse_interpolated_symbol(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    if (current_token().type() == Token::Type::InterpolatedSymbolEnd) {
        auto symbol = new SymbolNode { token, new String };
        advance();
        return symbol;
    } else if (current_token().type() == Token::Type::String && peek_token().type() == Token::Type::InterpolatedSymbolEnd) {
        auto symbol = new SymbolNode { token, current_token().literal_string() };
        advance();
        advance();
        return symbol;
    } else {
        auto interpolated_symbol = new InterpolatedSymbolNode { token };
        parse_interpolated_body(locals, interpolated_symbol, Token::Type::InterpolatedSymbolEnd);
        advance();
        return interpolated_symbol;
    }
};

Node *Parser::parse_lit(LocalsHashmap &) {
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::Bignum:
        advance();
        return new BignumNode { token, token.literal_string() };
    case Token::Type::Fixnum:
        advance();
        return new FixnumNode { token, token.get_fixnum() };
    case Token::Type::Float:
        advance();
        return new FloatNode { token, token.get_double() };
    default:
        TM_UNREACHABLE();
    }
};

Node *Parser::parse_keyword_splat(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    return new KeywordSplatNode { token, parse_expression(Precedence::SPLAT, locals) };
}

SharedPtr<String> Parser::parse_method_name(LocalsHashmap &) {
    SharedPtr<String> name = new String("");
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::BareName:
    case Token::Type::Constant:
        name = current_token().literal_string();
        break;
    default:
        if (token.is_operator() || token.is_keyword())
            name = new String(current_token().type_value());
        else
            throw_unexpected("method name");
    }
    advance();
    return name;
}

Node *Parser::parse_module(LocalsHashmap &) {
    auto token = current_token();
    advance();
    LocalsHashmap our_locals { TM::HashType::TMString };
    auto name = parse_class_or_module_name(our_locals);
    auto body = parse_body(our_locals, Precedence::LOWEST, Token::Type::EndKeyword, true);
    expect(Token::Type::EndKeyword, "module end");
    advance();
    return new ModuleNode { token, name, body };
}

Node *Parser::parse_next(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    if (current_token().is_lparen()) {
        advance();
        if (current_token().is_rparen()) {
            advance();
            return new NextNode { token, new NilSexpNode { token } };
        } else {
            auto arg = parse_expression(Precedence::BARE_CALL_ARG, locals);
            expect(Token::Type::RParen, "break closing paren");
            advance();
            return new NextNode { token, arg };
        }
    } else if (current_token().can_be_first_arg_of_implicit_call()) {
        auto value = parse_expression(Precedence::BARE_CALL_ARG, locals);
        if (current_token().is_comma()) {
            auto array = new ArrayNode { token };
            array->add_node(value);
            while (current_token().is_comma()) {
                advance();
                array->add_node(parse_expression(Precedence::BARE_CALL_ARG, locals));
            }
            value = array;
        }
        return new NextNode { token, value };
    }
    return new NextNode { token };
}

Node *Parser::parse_nil(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new NilSexpNode { token };
}

Node *Parser::parse_not(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto precedence = get_precedence(token);
    auto node = new NotNode {
        token,
        parse_expression(precedence, locals),
    };
    return node;
}

Node *Parser::parse_nth_ref(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new NthRefNode { token, token.get_fixnum() };
}

Node *Parser::parse_redo(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new RedoNode { token };
}

Node *Parser::parse_retry(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new RetryNode { token };
}

Node *Parser::parse_return(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    Node *value;
    if (current_token().is_end_of_expression()) {
        value = new NilNode { token };
    } else {
        value = parse_expression(Precedence::BARE_CALL_ARG, locals);
    }
    if (current_token().is_comma()) {
        auto array = new ArrayNode { current_token() };
        array->add_node(value);
        while (current_token().is_comma()) {
            advance();
            array->add_node(parse_expression(Precedence::BARE_CALL_ARG, locals));
        }
        value = array;
    }
    return new ReturnNode { token, value };
};

Node *Parser::parse_sclass(LocalsHashmap &locals) {
    auto token = current_token();
    advance(); // class
    advance(); // <<
    auto klass = parse_expression(Precedence::BARE_CALL_ARG, locals);
    auto body = parse_body(locals, Precedence::LOWEST);
    expect(Token::Type::EndKeyword, "sclass end");
    advance();
    return new SclassNode { token, klass, body };
}

Node *Parser::parse_self(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new SelfNode { token };
};

Node *Parser::parse_splat(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    return new SplatNode { token, parse_expression(Precedence::SPLAT, locals) };
};

Node *Parser::parse_stabby_proc(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    bool has_args = false;
    SharedPtr<Vector<Node *>> args = new Vector<Node *> {};
    if (current_token().is_lparen()) {
        has_args = true;
        advance();
        if (current_token().is_rparen()) {
            advance();
        } else {
            args = parse_def_args(locals);
            expect(Token::Type::RParen, "proc args closing paren");
            advance();
        }
    } else if (current_token().is_bare_name() || current_token().type() == Token::Type::Multiply) {
        has_args = true;
        args = parse_def_args(locals);
    }
    if (current_token().type() != Token::Type::DoKeyword && current_token().type() != Token::Type::LCurlyBrace)
        throw_unexpected("block");
    auto left = new StabbyProcNode { token, has_args, *args };
    return parse_iter_expression(left, locals);
};

Node *Parser::parse_string(LocalsHashmap &locals) {
    auto string_token = current_token();
    Node *string = new StringNode { string_token, string_token.literal_string() };
    advance();

    bool adjacent_strings_were_appended = false;
    if (m_precedence_stack.is_empty() || m_precedence_stack.last() != Precedence::WORD_ARRAY)
        string = concat_adjacent_strings(string, locals, adjacent_strings_were_appended);

    if (!adjacent_strings_were_appended && current_token().type() == Token::Type::TernaryColon && !current_token().whitespace_precedes()) {
        advance();
        return convert_string_to_symbol_key(string);
    }

    return string;
};

Node *Parser::concat_adjacent_strings(Node *string, LocalsHashmap &locals, bool &strings_were_appended) {
    auto token = current_token();
    while (token.type() == Token::Type::String || token.type() == Token::Type::InterpolatedStringBegin) {
        switch (token.type()) {
        case Token::Type::String: {
            auto next_string = parse_string(locals);
            string = append_string_nodes(string, next_string);
            break;
        }
        case Token::Type::InterpolatedStringBegin: {
            auto next_string = parse_interpolated_string(locals);
            string = append_string_nodes(string, next_string);
            break;
        }
        default:
            TM_UNREACHABLE();
        }
        token = current_token();
        strings_were_appended = true;
    }
    return string;
}

Node *Parser::append_string_nodes(Node *string1, Node *string2) {
    switch (string1->type()) {
    case Node::Type::String: {
        auto string1_node = static_cast<StringNode *>(string1);
        switch (string2->type()) {
        case Node::Type::String: {
            auto string2_node = static_cast<StringNode *>(string2);
            string1_node->string()->append(*string2_node->string());
            delete string2;
            return string1;
        }
        case Node::Type::InterpolatedString: {
            auto string2_node = static_cast<InterpolatedStringNode *>(string2);
            assert(!string2_node->is_empty());
            assert(string2_node->nodes().first()->type() == Node::Type::String);
            static_cast<StringNode *>(string2_node->nodes().first())->string()->prepend(*string1_node->string());
            delete string1;
            return string2;
        }
        default:
            TM_UNREACHABLE();
        }
        break;
    }
    case Node::Type::InterpolatedString: {
        auto string1_node = static_cast<InterpolatedStringNode *>(string1);
        switch (string2->type()) {
        case Node::Type::String: {
            auto string2_node = static_cast<StringNode *>(string2);
            assert(!string1_node->is_empty());
            auto last_node = string1_node->nodes().last();
            switch (last_node->type()) {
            case Node::Type::String:
                if (string1_node->nodes().size() > 1) {
                    // For some reason, RubyParser doesn't append two string nodes
                    // if there is an evstr present.
                    string1_node->add_node(string2);
                } else {
                    static_cast<StringNode *>(last_node)->string()->append(*string2_node->string());
                    delete string2;
                }
                return string1;
            case Node::Type::EvaluateToString:
                string1_node->add_node(string2);
                return string1;
            default:
                TM_UNREACHABLE();
            }
        }
        case Node::Type::InterpolatedString: {
            auto string2_node = static_cast<InterpolatedStringNode *>(string2);
            assert(!string1_node->is_empty());
            assert(!string2_node->is_empty());
            for (auto node : string2_node->nodes()) {
                string1_node->add_node(node->clone());
            }
            delete string2;
            return string1;
        }
        default:
            TM_UNREACHABLE();
        }
        break;
    }
    default:
        TM_UNREACHABLE();
    }
    return string1;
}

Node *Parser::parse_super(LocalsHashmap &) {
    auto token = current_token();
    advance();
    auto node = new SuperNode { token };
    if (current_token().is_lparen())
        node->set_parens(true);
    return node;
};

Node *Parser::parse_symbol(LocalsHashmap &) {
    auto token = current_token();
    auto symbol = new SymbolNode { token, current_token().literal_string() };
    advance();
    return symbol;
};

Node *Parser::parse_symbol_key(LocalsHashmap &) {
    auto token = current_token();
    auto symbol = new SymbolKeyNode { token, current_token().literal_string() };
    advance();
    return symbol;
};

Node *Parser::parse_top_level_constant(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    SharedPtr<String> name = new String("");
    auto name_token = current_token();
    auto identifier = static_cast<IdentifierNode *>(parse_identifier(locals));
    switch (identifier->token_type()) {
    case Token::Type::BareName:
    case Token::Type::Constant:
        name = identifier->name();
        break;
    default:
        throw_unexpected(name_token, ":: identifier name");
    }
    return new Colon3Node { token, name };
}

Node *Parser::parse_unary_operator(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto precedence = get_precedence(token);
    auto receiver = parse_expression(precedence, locals);
    if ((token.type() == Token::Type::Minus || token.type() == Token::Type::Plus) && receiver->is_numeric()) {
        switch (receiver->type()) {
        case Node::Type::Bignum: {
            if (token.type() == Token::Type::Minus) {
                auto num = static_cast<BignumNode *>(receiver);
                num->negate();
                return num;
            }
            return receiver;
        }
        case Node::Type::Fixnum: {
            if (token.type() == Token::Type::Minus) {
                auto num = static_cast<FixnumNode *>(receiver);
                num->negate();
                return num;
            }
            return receiver;
        }
        case Node::Type::Float: {
            if (token.type() == Token::Type::Minus) {
                auto num = static_cast<FloatNode *>(receiver);
                num->negate();
                return num;
            }
            return receiver;
        }
        default:
            TM_UNREACHABLE();
        }
    }
    SharedPtr<String> message = new String("");
    switch (token.type()) {
    case Token::Type::Minus:
        *message = "-@";
        break;
    case Token::Type::Plus:
        *message = "+@";
        break;
    case Token::Type::Tilde:
        *message = "~";
        break;
    default:
        TM_UNREACHABLE();
    }
    return new UnaryOpNode { token, message, receiver };
}

Node *Parser::parse_undef(LocalsHashmap &locals) {
    auto undef_token = current_token();
    advance();
    auto symbol_from_token = [&](Token &token) {
        switch (token.type()) {
        case Token::Type::BareName:
        case Token::Type::Constant:
            advance();
            return new SymbolNode { token, token.literal_string() };
        case Token::Type::Symbol:
            return static_cast<SymbolNode *>(parse_symbol(locals));
        case Token::Type::InterpolatedSymbolBegin: {
            return static_cast<SymbolNode *>(parse_interpolated_symbol(locals));
        }
        default:
            throw_unexpected("method name for undef");
        }
    };
    auto undef_node = new UndefNode { undef_token };
    auto token = current_token();
    undef_node->add_arg(symbol_from_token(token));
    if (current_token().is_comma()) {
        auto block = new BlockNode { undef_token };
        block->add_node(undef_node);
        while (current_token().is_comma()) {
            advance();
            token = current_token();
            undef_node = new UndefNode { undef_token };
            undef_node->add_arg(symbol_from_token(token));
            block->add_node(undef_node);
        }
        return block;
    }
    return undef_node;
};

Node *Parser::parse_word_array(LocalsHashmap &locals) {
    auto token = current_token();
    auto array = new ArrayNode { token };
    advance();
    while (!current_token().is_eof() && current_token().type() != Token::Type::RBracket)
        array->add_node(parse_expression(Precedence::WORD_ARRAY, locals));
    expect(Token::Type::RBracket, "closing array bracket");
    advance();
    return array;
}

Node *Parser::parse_word_symbol_array(LocalsHashmap &locals) {
    auto token = current_token();
    auto array = new ArrayNode { token };
    advance();
    while (!current_token().is_eof() && current_token().type() != Token::Type::RBracket) {
        auto string = parse_expression(Precedence::WORD_ARRAY, locals);
        assert(string->type() == Node::Type::String);
        auto symbol = new SymbolNode { string->token(), static_cast<StringNode *>(string)->string() };
        delete string;
        array->add_node(symbol);
    }
    expect(Token::Type::RBracket, "closing array bracket");
    advance();
    return array;
}

Node *Parser::parse_yield(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new YieldNode { token };
};

Node *Parser::parse_assignment_expression(Node *left, LocalsHashmap &locals) {
    return parse_assignment_expression(left, locals, true);
}

Node *Parser::parse_assignment_expression_without_multiple_values(Node *left, LocalsHashmap &locals) {
    return parse_assignment_expression(left, locals, false);
}

Node *Parser::parse_assignment_expression(Node *left, LocalsHashmap &locals, bool allow_multiple) {
    auto token = current_token();
    if (left->type() == Node::Type::Splat) {
        left = parse_multiple_assignment_expression(left, locals);
    }
    switch (left->type()) {
    case Node::Type::Identifier: {
        auto left_identifier = static_cast<IdentifierNode *>(left);
        left_identifier->add_to_locals(locals);
        advance();
        auto value = parse_assignment_expression_value(false, locals, allow_multiple);
        return new AssignmentNode { token, left, value };
    }
    case Node::Type::Call:
    case Node::Type::Colon2:
    case Node::Type::Colon3: {
        advance();
        auto value = parse_assignment_expression_value(false, locals, allow_multiple);
        return new AssignmentNode { token, left, value };
    }
    case Node::Type::MultipleAssignment: {
        static_cast<MultipleAssignmentNode *>(left)->add_locals(locals);
        advance();
        auto value = parse_assignment_expression_value(true, locals, allow_multiple);
        return new AssignmentNode { token, left, value };
    }
    default:
        throw_unexpected(left->token(), "left side of assignment");
    }
};

Node *Parser::parse_assignment_expression_value(bool to_array, LocalsHashmap &locals, bool allow_multiple) {
    auto token = current_token();
    auto value = parse_expression(Precedence::ASSIGNMENT_RHS, locals);
    bool is_splat;

    if (allow_multiple && current_token().type() == Token::Type::Comma) {
        auto array = new ArrayNode { token };
        array->add_node(value);
        while (current_token().type() == Token::Type::Comma) {
            advance();
            array->add_node(parse_expression(Precedence::ASSIGNMENT_RHS, locals));
        }
        value = array;
        is_splat = true;
    } else if (value->type() == Node::Type::Splat) {
        is_splat = true;
    } else {
        is_splat = false;
    }

    if (is_splat) {
        if (to_array) {
            return value;
        } else {
            return new SplatValueNode { token, value };
        }
    } else {
        if (to_array) {
            return new ToArrayNode { token, value };
        } else {
            return value;
        }
    }
}

Node *Parser::parse_iter_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    LocalsHashmap our_locals { locals }; // copy!
    bool curly_brace = current_token().type() == Token::Type::LCurlyBrace;
    bool has_args = false;
    SharedPtr<Vector<Node *>> args = new Vector<Node *> {};
    if (left->type() == Node::Type::StabbyProc) {
        advance();
        auto stabby_proc_node = static_cast<StabbyProcNode *>(left);
        has_args = stabby_proc_node->has_args();
        for (auto arg : stabby_proc_node->args())
            args->push(arg->clone());
    } else if (left->can_accept_a_block()) {
        advance();
        if (current_token().type() == Token::Type::Or) {
            has_args = true;
            advance();
        } else if (current_token().is_block_arg_delimiter()) {
            has_args = true;
            advance();
            parse_iter_args(args, our_locals);
            expect(Token::Type::BitwiseOr, "end of block args");
            advance();
        }
    } else {
        throw_unexpected(left->token(), "call to accept block");
    }
    auto body = parse_iter_body(our_locals, curly_brace);
    auto end_token_type = curly_brace ? Token::Type::RCurlyBrace : Token::Type::EndKeyword;
    expect(end_token_type, curly_brace ? "}" : "end");
    advance();
    return new IterNode { token, left, has_args, *args, body };
}

void Parser::parse_iter_args(SharedPtr<Vector<Node *>> args, LocalsHashmap &locals) {
    args->push(parse_def_single_arg(locals));
    while (current_token().is_comma()) {
        advance();
        if (current_token().is_block_arg_delimiter()) {
            // trailing comma with no additional arg
            args->push(new NilNode { current_token() });
            break;
        }
        args->push(parse_def_single_arg(locals));
    }
}

BlockNode *Parser::parse_iter_body(LocalsHashmap &locals, bool curly_brace) {
    auto end_token_type = curly_brace ? Token::Type::RCurlyBrace : Token::Type::EndKeyword;
    return parse_body(locals, Precedence::LOWEST, end_token_type, true);
}

Node *Parser::parse_call_expression_with_parens(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    NodeWithArgs *call_node = left->to_node_with_args();
    advance();
    if (!current_token().is_rparen())
        parse_call_args(call_node, locals, false);
    expect(Token::Type::RParen, "call rparen");
    advance();
    return call_node;
}

void Parser::parse_call_args(NodeWithArgs *node, LocalsHashmap &locals, bool bare) {
    auto arg = parse_expression(bare ? Precedence::BARE_CALL_ARG : Precedence::CALL_ARG, locals);
    if (current_token().type() == Token::Type::HashRocket || arg->is_symbol_key()) {
        node->add_arg(parse_call_hash_args(locals, bare, arg));
    } else {
        node->add_arg(arg);
        while (current_token().is_comma()) {
            advance();
            auto token = current_token();
            if (token.is_rparen()) {
                // trailing comma with no additional arg
                break;
            }
            arg = parse_expression(bare ? Precedence::BARE_CALL_ARG : Precedence::CALL_ARG, locals);
            if (current_token().type() == Token::Type::HashRocket || arg->is_symbol_key()) {
                node->add_arg(parse_call_hash_args(locals, bare, arg));
                break;
            } else {
                node->add_arg(arg);
            }
        }
    }
}

Node *Parser::parse_call_hash_args(LocalsHashmap &locals, bool bare, Node *first_arg) {
    if (bare)
        return parse_hash_inner(locals, Precedence::BARE_CALL_ARG, Token::Type::Invalid, first_arg);
    else
        return parse_hash_inner(locals, Precedence::CALL_ARG, Token::Type::RParen, first_arg);
}

Node *Parser::parse_call_expression_without_parens(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    auto call_node = left->to_node_with_args();
    switch (token.type()) {
    case Token::Type::Comma:
    case Token::Type::Eof:
    case Token::Type::Eol:
    case Token::Type::RBracket:
    case Token::Type::RCurlyBrace:
    case Token::Type::RParen:
        break;
    default:
        parse_call_args(call_node, locals, true);
    }
    return call_node;
}

Node *Parser::parse_constant_resolution_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto name_token = current_token();
    auto identifier = static_cast<IdentifierNode *>(parse_identifier(locals));
    Node *node;
    switch (identifier->token_type()) {
    case Token::Type::BareName: {
        auto name = identifier->name();
        node = new CallNode { token, left, name };
        delete identifier;
        break;
    }
    case Token::Type::Constant: {
        auto name = identifier->name();
        node = new Colon2Node { token, left, name };
        delete identifier;
        break;
    }
    default:
        throw_unexpected(name_token, ":: identifier name");
    }
    return node;
}

Node *Parser::parse_infix_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    auto op = current_token();
    auto precedence = get_precedence(token, left);
    advance();
    auto right = parse_expression(precedence, locals);
    auto *node = new InfixOpNode {
        token,
        left,
        new String(op.type_value()),
        right,
    };
    return node;
};

Node *Parser::parse_logical_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::And: {
        advance();
        auto right = parse_expression(Precedence::LOGICAL_AND, locals);
        if (left->type() == Node::Type::LogicalAnd) {
            return regroup<LogicalAndNode>(token, left, right);
        } else {
            return new LogicalAndNode { token, left, right };
        }
    }
    case Token::Type::AndKeyword: {
        advance();
        auto right = parse_expression(Precedence::COMPOSITION, locals);
        if (left->type() == Node::Type::LogicalAnd) {
            return regroup<LogicalAndNode>(token, left, right);
        } else {
            return new LogicalAndNode { token, left, right };
        }
    }
    case Token::Type::Or: {
        advance();
        auto right = parse_expression(Precedence::LOGICAL_OR, locals);
        if (left->type() == Node::Type::LogicalOr) {
            return regroup<LogicalOrNode>(token, left, right);
        } else {
            return new LogicalOrNode { token, left, right };
        }
    }
    case Token::Type::OrKeyword: {
        advance();
        auto right = parse_expression(Precedence::COMPOSITION, locals);
        if (left->type() == Node::Type::LogicalOr) {
            return regroup<LogicalOrNode>(token, left, right);
        } else {
            return new LogicalOrNode { token, left, right };
        }
    }
    default:
        TM_UNREACHABLE();
    }
}

Node *Parser::parse_match_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto arg = parse_expression(Precedence::EQUALITY, locals);
    if (left->type() == Node::Type::Regexp) {
        return new MatchNode { token, static_cast<RegexpNode *>(left), arg, true };
    } else if (arg->type() == Node::Type::Regexp) {
        return new MatchNode { token, static_cast<RegexpNode *>(arg), left, false };
    } else {
        auto *node = new CallNode {
            token,
            left,
            new String("=~"),
        };
        node->add_arg(arg);
        return node;
    }
}

Node *Parser::parse_not_match_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    auto match = parse_match_expression(left, locals);
    return new NotMatchNode { token, match };
}

Node *Parser::parse_op_assign_expression(Node *left, LocalsHashmap &locals) {
    if (left->type() == Node::Type::Call)
        return parse_op_attr_assign_expression(left, locals);
    if (left->type() != Node::Type::Identifier)
        throw_unexpected(left->token(), "identifier");
    auto left_identifier = static_cast<IdentifierNode *>(left);
    left_identifier->set_is_lvar(true);
    left_identifier->add_to_locals(locals);
    auto token = current_token();
    advance();
    switch (token.type()) {
    case Token::Type::AndEqual:
        return new OpAssignAndNode { token, left_identifier, parse_expression(Precedence::ASSIGNMENT_RHS, locals) };
    case Token::Type::BitwiseAndEqual:
    case Token::Type::BitwiseOrEqual:
    case Token::Type::BitwiseXorEqual:
    case Token::Type::DivideEqual:
    case Token::Type::ExponentEqual:
    case Token::Type::LeftShiftEqual:
    case Token::Type::MinusEqual:
    case Token::Type::ModulusEqual:
    case Token::Type::MultiplyEqual:
    case Token::Type::PlusEqual:
    case Token::Type::RightShiftEqual: {
        auto op = new String(token.type_value());
        op->chomp();
        return new OpAssignNode { token, op, left_identifier, parse_expression(Precedence::ASSIGNMENT_RHS, locals) };
    }
    case Token::Type::OrEqual:
        return new OpAssignOrNode { token, left_identifier, parse_expression(Precedence::ASSIGNMENT_RHS, locals) };
    default:
        TM_UNREACHABLE();
    }
}

Node *Parser::parse_op_attr_assign_expression(Node *left, LocalsHashmap &locals) {
    if (left->type() != Node::Type::Call)
        throw_unexpected(left->token(), "call");
    auto left_call = static_cast<CallNode *>(left);
    auto token = current_token();
    advance();
    auto op = new String(token.type_value());
    op->chomp();
    SharedPtr<String> message = new String(left_call->message().ref());
    message->append_char('=');
    return new OpAssignAccessorNode {
        token,
        op,
        left_call->receiver().clone(),
        message,
        parse_expression(Precedence::OP_ASSIGNMENT, locals),
        left_call->args(),
    };
}

Node *Parser::parse_proc_call_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    advance();
    auto call_node = new CallNode {
        token,
        left,
        new String("call"),
    };
    if (!current_token().is_rparen())
        parse_call_args(call_node, locals, false);
    expect(Token::Type::RParen, "proc call right paren");
    advance();
    return call_node;
}

Node *Parser::parse_range_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    Node *right;
    try {
        right = parse_expression(Precedence::RANGE, locals);
    } catch (SyntaxError &e) {
        // NOTE: I'm not sure if this is the "right" way to handle an endless range,
        // but it seems to be effective for the tests I threw at it. ¯\_(ツ)_/¯
        right = new NilNode { token };
    }
    return new RangeNode { token, left, right, token.type() == Token::Type::DotDotDot };
}

Node *Parser::parse_ref_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto call_node = new CallNode {
        token,
        left,
        new String("[]"),
    };
    if (token.type() == Token::Type::LBracketRBracket)
        return call_node;
    if (current_token().type() != Token::Type::RBracket)
        parse_call_args(call_node, locals, false);
    expect(Token::Type::RBracket, "element reference right bracket");
    advance();
    return call_node;
}

Node *Parser::parse_rescue_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto value = parse_expression(Precedence::LOWEST, locals);
    auto body = new BlockNode { left->token(), left };
    auto begin_node = new BeginNode { token, body };
    auto rescue_node = new BeginRescueNode { token };
    rescue_node->set_body(new BlockNode { value->token(), value });
    begin_node->add_rescue_node(rescue_node);
    return begin_node;
}

Node *Parser::parse_safe_send_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    expect(Token::Type::BareName, "safe navigator method name");
    auto name = static_cast<IdentifierNode *>(parse_identifier(locals));
    auto call_node = new SafeCallNode {
        token,
        left,
        name->name(),
    };
    return call_node;
}

Node *Parser::parse_send_expression(Node *left, LocalsHashmap &locals) {
    auto dot_token = current_token();
    advance();
    auto name_token = current_token();
    SharedPtr<String> name = new String("");
    switch (name_token.type()) {
    case Token::Type::BareName: {
        auto identifier = static_cast<IdentifierNode *>(parse_identifier(locals));
        name = identifier->name();
        delete identifier;
        break;
    }
    case Token::Type::Constant:
        name = name_token.literal_string();
        advance();
        break;
    case Token::Type::ClassKeyword:
    case Token::Type::BeginKeyword:
    case Token::Type::EndKeyword:
        *name = name_token.type_value();
        advance();
        break;
    default:
        if (name_token.is_operator() || name_token.is_keyword()) {
            *name = name_token.type_value();
            advance();
        } else {
            throw_unexpected("send method name");
        }
    };
    return new CallNode {
        dot_token,
        left,
        name,
    };
}

Node *Parser::parse_ternary_expression(Node *left, LocalsHashmap &locals) {
    auto token = current_token();
    expect(Token::Type::TernaryQuestion, "ternary question");
    advance();
    auto true_expr = parse_expression(Precedence::TERNARY_TRUE, locals);
    expect(Token::Type::TernaryColon, "ternary colon");
    advance();
    auto false_expr = parse_expression(Precedence::TERNARY_FALSE, locals);
    return new IfNode { token, left, true_expr, false_expr };
}

Node *Parser::parse_unless(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto condition = parse_expression(Precedence::LOWEST, locals);
    next_expression();
    auto false_expr = parse_if_body(locals);
    Node *true_expr;
    if (current_token().is_else_keyword()) {
        advance();
        true_expr = parse_if_body(locals);
    } else {
        true_expr = new NilNode { current_token() };
    }
    expect(Token::Type::EndKeyword, "unless end");
    advance();
    return new IfNode { token, condition, true_expr, false_expr };
}

Node *Parser::parse_while(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto condition = parse_expression(Precedence::LOWEST, locals);
    next_expression();
    auto body = parse_body(locals, Precedence::LOWEST);
    expect(Token::Type::EndKeyword, "while end");
    advance();
    switch (token.type()) {
    case Token::Type::UntilKeyword:
        return new UntilNode { token, condition, body, true };
    case Token::Type::WhileKeyword:
        return new WhileNode { token, condition, body, true };
    default:
        TM_UNREACHABLE();
    }
}

Parser::parse_null_fn Parser::null_denotation(Token::Type type) {
    using Type = Token::Type;
    switch (type) {
    case Type::AliasKeyword:
        return &Parser::parse_alias;
    case Type::LBracket:
    case Type::LBracketRBracket:
        return &Parser::parse_array;
    case Type::BackRef:
        return &Parser::parse_back_ref;
    case Type::BeginKeyword:
        return &Parser::parse_begin;
    case Type::BitwiseAnd:
        return &Parser::parse_block_pass;
    case Type::TrueKeyword:
    case Type::FalseKeyword:
        return &Parser::parse_bool;
    case Type::BreakKeyword:
        return &Parser::parse_break;
    case Type::CaseKeyword:
        return &Parser::parse_case;
    case Type::ClassKeyword:
        return &Parser::parse_class;
    case Type::DefKeyword:
        return &Parser::parse_def;
    case Type::DefinedKeyword:
        return &Parser::parse_defined;
    case Type::DotDot:
    case Type::DotDotDot:
        return &Parser::parse_beginless_range;
    case Type::FILEKeyword:
        return &Parser::parse_file_constant;
    case Type::LParen:
        return &Parser::parse_group;
    case Type::LCurlyBrace:
        return &Parser::parse_hash;
    case Type::BareName:
    case Type::ClassVariable:
    case Type::Constant:
    case Type::GlobalVariable:
    case Type::InstanceVariable:
        return &Parser::parse_identifier;
    case Type::IfKeyword:
        return &Parser::parse_if;
    case Type::InterpolatedRegexpBegin:
        return &Parser::parse_interpolated_regexp;
    case Type::InterpolatedShellBegin:
        return &Parser::parse_interpolated_shell;
    case Type::InterpolatedStringBegin:
        return &Parser::parse_interpolated_string;
    case Type::InterpolatedSymbolBegin:
        return &Parser::parse_interpolated_symbol;
    case Type::Exponent:
        return &Parser::parse_keyword_splat;
    case Type::Bignum:
    case Type::Fixnum:
    case Type::Float:
        return &Parser::parse_lit;
    case Type::ModuleKeyword:
        return &Parser::parse_module;
    case Type::NextKeyword:
        return &Parser::parse_next;
    case Type::NilKeyword:
        return &Parser::parse_nil;
    case Type::Not:
    case Type::NotKeyword:
        return &Parser::parse_not;
    case Type::NthRef:
        return &Parser::parse_nth_ref;
    case Type::RedoKeyword:
        return &Parser::parse_redo;
    case Type::RetryKeyword:
        return &Parser::parse_retry;
    case Type::ReturnKeyword:
        return &Parser::parse_return;
    case Type::SelfKeyword:
        return &Parser::parse_self;
    case Type::Multiply:
        return &Parser::parse_splat;
    case Type::Arrow:
        return &Parser::parse_stabby_proc;
    case Type::String:
        return &Parser::parse_string;
    case Type::SuperKeyword:
        return &Parser::parse_super;
    case Type::Symbol:
        return &Parser::parse_symbol;
    case Type::SymbolKey:
        return &Parser::parse_symbol_key;
    case Type::ConstantResolution:
        return &Parser::parse_top_level_constant;
    case Type::Minus:
    case Type::Plus:
    case Type::Tilde:
        return &Parser::parse_unary_operator;
    case Type::UndefKeyword:
        return &Parser::parse_undef;
    case Type::UnlessKeyword:
        return &Parser::parse_unless;
    case Type::UntilKeyword:
    case Type::WhileKeyword:
        return &Parser::parse_while;
    case Type::PercentLowerI:
    case Type::PercentUpperI:
        return &Parser::parse_word_symbol_array;
    case Type::PercentLowerW:
    case Type::PercentUpperW:
        return &Parser::parse_word_array;
    case Type::YieldKeyword:
        return &Parser::parse_yield;
    default:
        return nullptr;
    }
}

Parser::parse_left_fn Parser::left_denotation(Token &token, Node *left, Precedence precedence) {
    using Type = Token::Type;
    switch (token.type()) {
    case Type::Equal:
        if (precedence == Precedence::ARRAY || precedence == Precedence::BARE_CALL_ARG || precedence == Precedence::CALL_ARG)
            return &Parser::parse_assignment_expression_without_multiple_values;
        else
            return &Parser::parse_assignment_expression;
    case Type::LParen:
        return &Parser::parse_call_expression_with_parens;
    case Type::ConstantResolution:
        return &Parser::parse_constant_resolution_expression;
    case Type::BitwiseAnd:
    case Type::BitwiseOr:
    case Type::BitwiseXor:
    case Type::Comparison:
    case Type::Divide:
    case Type::EqualEqual:
    case Type::EqualEqualEqual:
    case Type::Exponent:
    case Type::GreaterThan:
    case Type::GreaterThanOrEqual:
    case Type::LeftShift:
    case Type::LessThan:
    case Type::LessThanOrEqual:
    case Type::Minus:
    case Type::Modulus:
    case Type::Multiply:
    case Type::NotEqual:
    case Type::Plus:
    case Type::RightShift:
        return &Parser::parse_infix_expression;
    case Type::DoKeyword:
    case Type::LCurlyBrace:
        return &Parser::parse_iter_expression;
    case Type::And:
    case Type::AndKeyword:
    case Type::Or:
    case Type::OrKeyword:
        return &Parser::parse_logical_expression;
    case Type::Match:
        return &Parser::parse_match_expression;
    case Type::IfKeyword:
    case Type::UnlessKeyword:
    case Type::WhileKeyword:
    case Type::UntilKeyword:
        return &Parser::parse_modifier_expression;
    case Type::Comma:
        return &Parser::parse_multiple_assignment_expression;
    case Type::NotMatch:
        return &Parser::parse_not_match_expression;
    case Type::AndEqual:
    case Type::BitwiseAndEqual:
    case Type::BitwiseOrEqual:
    case Type::BitwiseXorEqual:
    case Type::DivideEqual:
    case Type::ExponentEqual:
    case Type::LeftShiftEqual:
    case Type::MinusEqual:
    case Type::ModulusEqual:
    case Type::MultiplyEqual:
    case Type::OrEqual:
    case Type::PlusEqual:
    case Type::RightShiftEqual:
        return &Parser::parse_op_assign_expression;
    case Type::DotDot:
    case Type::DotDotDot:
        return &Parser::parse_range_expression;
    case Type::LBracket:
    case Type::LBracketRBracket: {
        if (treat_left_bracket_as_element_reference(left, token))
            return &Parser::parse_ref_expression;
        break;
    }
    case Type::RescueKeyword:
        return &Parser::parse_rescue_expression;
    case Type::SafeNavigation:
        return &Parser::parse_safe_send_expression;
    case Type::Dot:
        if (peek_token().is_lparen()) {
            return &Parser::parse_proc_call_expression;
        } else {
            return &Parser::parse_send_expression;
        }
    case Type::TernaryQuestion:
        return &Parser::parse_ternary_expression;
    case Type::TernaryColon:
        TM_UNREACHABLE();
    default:
        break;
    }
    if (is_first_arg_of_call_without_parens(left, token))
        return &Parser::parse_call_expression_without_parens;
    return nullptr;
}

bool Parser::is_first_arg_of_call_without_parens(Node *left, Token &token) {
    return left->is_callable() && token.can_be_first_arg_of_implicit_call();
}

Token Parser::current_token() const {
    if (m_index < m_tokens->size()) {
        return m_tokens->at(m_index);
    } else {
        return Token::invalid();
    }
}

Token Parser::peek_token() const {
    if (m_index + 1 < m_tokens->size()) {
        return (*m_tokens)[m_index + 1];
    } else {
        return Token::invalid();
    }
}

void Parser::next_expression() {
    auto token = current_token();
    if (!token.is_end_of_expression())
        throw_unexpected("end-of-line");
    skip_newlines();
}

void Parser::skip_newlines() {
    while (current_token().is_eol())
        advance();
}

void Parser::expect(Token::Type type, const char *expected) {
    if (current_token().type() != type)
        throw_unexpected(expected);
}

void Parser::throw_unexpected(const Token &token, const char *expected) {
    auto file = token.file() ? String(*token.file()) : String("(unknown)");
    auto line = token.line() + 1;
    auto type = token.type_value();
    auto literal = token.literal();
    String message;
    if (token.type() == Token::Type::Invalid) {
        message = String::format("{}#{}: syntax error, unexpected '{}' (expected: '{}')", file, line, token.literal(), expected);
    } else if (!type) {
        message = String::format("{}#{}: syntax error, expected '{}' (token type: {})", file, line, expected, (long long)token.type());
    } else if (token.type() == Token::Type::Eof) {
        auto indent = String { token.column(), ' ' };
        message = String::format(
            "{}#{}: syntax error, unexpected end-of-input (expected: '{}')\n"
            "{}\n"
            "{}^ here, expected '{}'",
            file, line, expected, current_line(), indent, expected);
    } else if (literal) {
        auto indent = String { token.column(), ' ' };
        message = String::format(
            "{}#{}: syntax error, unexpected {} '{}' (expected: '{}')\n"
            "{}\n"
            "{}^ here, expected '{}'",
            file, line, type, literal, expected, current_line(), indent, expected);
    } else {
        auto indent = String { token.column(), ' ' };
        message = String::format(
            "{}#{}: syntax error, unexpected '{}' (expected: '{}')\n"
            "{}\n"
            "{}^ here, expected '{}'",
            file, line, type, expected, current_line(), indent, expected);
    }
    throw SyntaxError { message };
}

void Parser::throw_unexpected(const char *expected) {
    throw_unexpected(current_token(), expected);
}

String Parser::current_line() {
    size_t line = 0;
    size_t current_line = current_token().line();
    String buf;
    for (size_t i = 0; i < m_code->size(); ++i) {
        char c = (*m_code)[i];
        if (line == current_line && c != '\n')
            buf.append_char(c);
        else if (line > current_line)
            break;
        if (c == '\n')
            line++;
    }
    return buf;
}

}
