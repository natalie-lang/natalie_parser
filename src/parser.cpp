#include "natalie_parser/parser.hpp"
#include "natalie_parser/creator/debug_creator.hpp"

namespace NatalieParser {

enum class Parser::Precedence {
    LOWEST,
    ARRAY, // []
    WORD_ARRAY, // %w[]
    HASH, // {}
    CASE, // case/when/else
    COMPOSITION, // and/or
    EXPR_MODIFIER, // if/unless/while/until
    TERNARY_FALSE, // _ ? _ : (_)
    ASSIGNMENT_RHS, // a = (_)
    INLINE_RESCUE, // foo rescue 2
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

bool Parser::higher_precedence(Token &token, SharedPtr<Node> left, Precedence current_precedence) {
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

Parser::Precedence Parser::get_precedence(Token &token, SharedPtr<Node> left) {
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
        return Precedence::EXPR_MODIFIER;
    case Token::Type::RescueKeyword:
        return Precedence::INLINE_RESCUE;
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

SharedPtr<Node> Parser::parse_expression(Parser::Precedence precedence, LocalsHashmap &locals) {
    skip_newlines();

    m_precedence_stack.push(precedence);

    auto null_fn = null_denotation(current_token().type());
    if (!null_fn)
        throw_unexpected("expression");

    auto left = (this->*null_fn)(locals);

    while (current_token().is_valid()) {
        auto token = current_token();
        if (!higher_precedence(token, left, precedence))
            break;
        auto left_fn = left_denotation(token, left, precedence);
        if (!left_fn)
            throw_unexpected(token, "expression");
        left = (this->*left_fn)(left, locals);
        m_precedence_stack.pop();
        m_precedence_stack.push(precedence);
    }

    m_precedence_stack.pop();

    return left;
}

SharedPtr<Node> Parser::tree() {
    SharedPtr<Node> tree = new BlockNode { current_token() };
    validate_current_token();
    LocalsHashmap locals { TM::HashType::TMString };
    skip_newlines();
    while (!current_token().is_eof()) {
        auto exp = parse_expression(Precedence::LOWEST, locals);
        tree->as_block_node().add_node(exp);
        validate_current_token();
        next_expression();
    }
    if (tree->as_block_node().has_one_node())
        tree = tree->as_block_node().take_first_node();
    return tree;
}

SharedPtr<BlockNode> Parser::parse_body(LocalsHashmap &locals, Precedence precedence, Token::Type end_token_type, bool allow_rescue) {
    SharedPtr<BlockNode> body = new BlockNode { current_token() };
    validate_current_token();
    skip_newlines();
    while (!current_token().is_eof() && current_token().type() != end_token_type) {
        if (allow_rescue && current_token().type() == Token::Type::RescueKeyword) {
            auto token = body->token();
            SharedPtr<BeginNode> begin_node = new BeginNode { body->token(), body };
            parse_rest_of_begin(begin_node.ref(), locals);
            rewind(); // so the 'end' keyword can be consumed by our caller
            return new BlockNode { token, begin_node.static_cast_as<Node>() };
        }
        auto exp = parse_expression(precedence, locals);
        body->add_node(exp);
        next_expression();
    }
    return body;
}

// FIXME: Maybe pass a lambda in that can just compare the two types? (No vector needed.)
SharedPtr<BlockNode> Parser::parse_body(LocalsHashmap &locals, Precedence precedence, Vector<Token::Type> &end_tokens, const char *expected_message) {
    SharedPtr<BlockNode> body = new BlockNode { current_token() };
    validate_current_token();
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
        validate_current_token();
        next_expression();
    }
    if (!finished())
        throw_unexpected(expected_message);
    return body;
}

SharedPtr<BlockNode> Parser::parse_def_body(LocalsHashmap &locals) {
    auto token = current_token();
    SharedPtr<BlockNode> body = new BlockNode { token };
    skip_newlines();
    while (!current_token().is_eof() && !current_token().is_end_keyword()) {
        if (current_token().type() == Token::Type::RescueKeyword) {
            SharedPtr<BeginNode> begin_node = new BeginNode { token, body };
            parse_rest_of_begin(begin_node.ref(), locals);
            rewind(); // so the 'end' keyword can be consumed by parse_def
            return new BlockNode { token, begin_node.static_cast_as<Node>() };
        }
        auto exp = parse_expression(Precedence::LOWEST, locals);
        body->add_node(exp);
        next_expression();
    }
    return body;
}

SharedPtr<Node> Parser::parse_alias(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    SharedPtr<SymbolNode> new_name = parse_alias_arg(locals, "alias new name (first argument)", false);
    auto existing_name = parse_alias_arg(locals, "alias existing name (second argument)", true);
    return new AliasNode { token, new_name, existing_name };
}

SharedPtr<SymbolNode> Parser::parse_alias_arg(LocalsHashmap &locals, const char *expected_message, bool reinsert_collapsed_newline) {
    auto token = current_token();
    switch (token.type()) {
        // TODO: handle Constant too
    case Token::Type::BareName: {
        advance();
        return new SymbolNode { token, token.literal_string() };
    }
    case Token::Type::Symbol:
        return parse_symbol(locals).static_cast_as<SymbolNode>();
    case Token::Type::InterpolatedSymbolBegin:
        return parse_interpolated_symbol(locals).static_cast_as<SymbolNode>();
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

SharedPtr<Node> Parser::parse_array(LocalsHashmap &locals) {
    SharedPtr<ArrayNode> array = new ArrayNode { current_token() };
    if (current_token().type() == Token::Type::LBracketRBracket) {
        advance();
        return array.static_cast_as<Node>();
    }
    advance();
    auto add_node = [&]() -> SharedPtr<Node> {
        auto token = current_token();
        if (token.type() == Token::Type::RBracket) {
            advance();
            return array.static_cast_as<Node>();
        }
        if (token.type() == Token::Type::SymbolKey) {
            array->add_node(parse_hash_inner(locals, Precedence::HASH, Token::Type::RBracket));
            expect(Token::Type::RBracket, "array closing bracket");
            advance();
            return array.static_cast_as<Node>();
        }
        auto value = parse_expression(Precedence::ARRAY, locals);
        token = current_token();
        if (token.is_hash_rocket()) {
            array->add_node(parse_hash_inner(locals, Precedence::HASH, Token::Type::RBracket, value));
            expect(Token::Type::RBracket, "array closing bracket");
            advance();
            return array.static_cast_as<Node>();
        }
        array->add_node(value);
        return {};
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
    return array.static_cast_as<Node>();
}

void Parser::parse_comma_separated_expressions(ArrayNode &array, LocalsHashmap &locals) {
    array.add_node(parse_expression(Precedence::ARRAY, locals));
    while (current_token().type() == Token::Type::Comma) {
        advance();
        array.add_node(parse_expression(Precedence::ARRAY, locals));
    }
}

SharedPtr<Node> Parser::parse_back_ref(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new BackRefNode { token, token.literal_string()->at(0) };
}

SharedPtr<Node> Parser::parse_begin(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    next_expression();
    auto begin_ending_tokens = Vector<Token::Type> { { Token::Type::RescueKeyword, Token::Type::ElseKeyword, Token::Type::EnsureKeyword, Token::Type::EndKeyword } };
    auto body = parse_body(locals, Precedence::LOWEST, begin_ending_tokens, "case: rescue, else, ensure, or end");

    SharedPtr<BeginNode> begin_node = new BeginNode { token, body };
    parse_rest_of_begin(begin_node.ref(), locals);

    // a begin/end with nothing else just becomes a BlockNode
    if (!(begin_node->has_rescue_nodes() || begin_node->has_else_body() || begin_node->has_ensure_body()))
        return begin_node->body().static_cast_as<Node>();

    return begin_node.static_cast_as<Node>();
}

void Parser::parse_rest_of_begin(BeginNode &begin_node, LocalsHashmap &locals) {
    auto rescue_ending_tokens = Vector<Token::Type> { { Token::Type::RescueKeyword, Token::Type::ElseKeyword, Token::Type::EnsureKeyword, Token::Type::EndKeyword } };
    auto else_ending_tokens = Vector<Token::Type> { { Token::Type::EnsureKeyword, Token::Type::EndKeyword } };
    while (!current_token().is_eof() && !current_token().is_end_keyword()) {
        switch (current_token().type()) {
        case Token::Type::RescueKeyword: {
            SharedPtr<BeginRescueNode> rescue_node = new BeginRescueNode { current_token() };
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
                SharedPtr<IdentifierNode> name;
                switch (current_token().type()) {
                case Token::Type::BareName:
                    name = new IdentifierNode { current_token(), current_token().literal_string() };
                    advance();
                    break;
                default:
                    throw_unexpected("exception name");
                }
                name->add_to_locals(locals);
                rescue_node->set_exception_name(name);
            }
            next_expression();
            auto body = parse_body(locals, Precedence::LOWEST, rescue_ending_tokens, "case: rescue, else, ensure, or end");
            rescue_node->set_body(body);
            begin_node.add_rescue_node(rescue_node);
            break;
        }
        case Token::Type::ElseKeyword: {
            advance();
            next_expression();
            auto body = parse_body(locals, Precedence::LOWEST, else_ending_tokens, "case: ensure or end");
            begin_node.set_else_body(body);
            break;
        }
        case Token::Type::EnsureKeyword: {
            advance();
            next_expression();
            auto body = parse_body(locals, Precedence::LOWEST);
            begin_node.set_ensure_body(body);
            break;
        }
        default:
            throw_unexpected("begin end");
        }
    }
    expect(Token::Type::EndKeyword, "begin/rescue/ensure end");
    advance();
}

SharedPtr<Node> Parser::parse_beginless_range(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto end_node = parse_expression(Precedence::LOWEST, locals);
    return new RangeNode {
        token,
        new NilNode { token },
        end_node,
        token.type() == Token::Type::DotDotDot
    };
}

SharedPtr<Node> Parser::parse_block_pass(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto value = parse_expression(Precedence::UNARY_PLUS, locals);
    return new BlockPassNode { token, value };
}

SharedPtr<Node> Parser::parse_bool(LocalsHashmap &) {
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

SharedPtr<Node> Parser::parse_break(LocalsHashmap &locals) {
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
            SharedPtr<ArrayNode> array = new ArrayNode { token };
            array->add_node(value);
            while (current_token().is_comma()) {
                advance();
                array->add_node(parse_expression(Precedence::BARE_CALL_ARG, locals));
            }
            value = array.static_cast_as<Node>();
        }
        return new BreakNode { token, value };
    }
    return new BreakNode { token };
}

SharedPtr<Node> Parser::parse_case(LocalsHashmap &locals) {
    auto case_token = current_token();
    advance();
    SharedPtr<Node> subject;
    if (current_token().type() == Token::Type::WhenKeyword) {
        subject = new NilNode { case_token };
    } else {
        subject = parse_expression(Precedence::CASE, locals);
        next_expression();
    }
    SharedPtr<CaseNode> node = new CaseNode { case_token, subject };
    while (!current_token().is_end_keyword()) {
        auto token = current_token();
        switch (token.type()) {
        case Token::Type::WhenKeyword: {
            advance();
            SharedPtr<ArrayNode> condition_array = new ArrayNode { token };
            parse_comma_separated_expressions(condition_array.ref(), locals);
            if (current_token().type() == Token::Type::ThenKeyword) {
                advance();
                skip_newlines();
            } else {
                next_expression();
            }
            auto body = parse_case_when_body(locals);
            auto when_node = new CaseWhenNode { token, condition_array.static_cast_as<Node>(), body };
            node->add_node(when_node);
            break;
        }
        case Token::Type::InKeyword: {
            advance();
            SharedPtr<Node> pattern = parse_case_in_patterns(locals);
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
            SharedPtr<BlockNode> body = parse_body(locals, Precedence::LOWEST);
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
    return node.static_cast_as<Node>();
}

SharedPtr<BlockNode> Parser::parse_case_in_body(LocalsHashmap &locals) {
    return parse_case_when_body(locals);
}

SharedPtr<Node> Parser::parse_case_in_pattern(LocalsHashmap &locals) {
    auto token = current_token();
    SharedPtr<Node> node;
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
    case Token::Type::Constant:
        node = parse_constant(locals);
        break;
    case Token::Type::LBracketRBracket:
        advance();
        node = new ArrayPatternNode { token };
        break;
    case Token::Type::LBracket: {
        // TODO: might need to keep track of and pass along precedence value?
        advance();
        SharedPtr<ArrayPatternNode> array = new ArrayPatternNode { token };
        if (current_token().type() == Token::Type::RBracket) {
            advance();
            node = array.static_cast_as<Node>();
            break;
        }
        array->add_node(parse_case_in_pattern(locals));
        while (current_token().is_comma()) {
            advance();
            array->add_node(parse_case_in_pattern(locals));
        }
        expect(Token::Type::RBracket, "array pattern closing bracket");
        advance();
        node = array.static_cast_as<Node>();
        break;
    }
    case Token::Type::LCurlyBrace: {
        advance();
        SharedPtr<HashPatternNode> hash = new HashPatternNode { token };
        if (current_token().type() == Token::Type::RCurlyBrace) {
            advance();
            node = hash.static_cast_as<Node>();
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
        node = hash.static_cast_as<Node>();
        break;
    }
    case Token::Type::Bignum:
    case Token::Type::Fixnum:
    case Token::Type::Float:
        node = parse_lit(locals);
        break;
    case Token::Type::Multiply: {
        auto splat_token = current_token();
        advance();
        SharedPtr<ArrayPatternNode> array = new ArrayPatternNode { token };
        switch (current_token().type()) {
        case Token::Type::BareName:
        case Token::Type::Constant: {
            auto name = new IdentifierNode { current_token(), true };
            name->prepend_to_name('*');
            array->add_node(name);
            advance();
            break;
        }
        default:
            array->add_node(new SplatNode { splat_token });
        }
        node = array.static_cast_as<Node>();
        break;
    }
    case Token::Type::String:
        node = parse_string(locals);
        break;
    case Token::Type::Symbol:
        node = parse_symbol(locals);
        break;
    default:
        throw_unexpected("case in pattern");
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

SharedPtr<Node> Parser::parse_case_in_patterns(LocalsHashmap &locals) {
    Vector<SharedPtr<Node>> patterns;
    patterns.push(parse_case_in_pattern(locals));
    while (current_token().type() == Token::Type::BitwiseOr) {
        advance();
        patterns.push(parse_case_in_pattern(locals));
    }
    while (current_token().is_comma()) {
        advance();
        auto last_pattern = patterns.last();
        auto next_pattern = parse_case_in_pattern(locals);
        if (last_pattern->type() == Node::Type::ArrayPattern) {
            last_pattern.static_cast_as<ArrayPatternNode>()->add_node(next_pattern);
        } else {
            patterns.pop();
            auto array_pattern = new ArrayPatternNode { last_pattern->token() };
            array_pattern->add_node(last_pattern);
            array_pattern->add_node(next_pattern);
            patterns.push(array_pattern);
        }
    }
    assert(patterns.size() > 0);
    if (patterns.size() == 1) {
        return patterns.pop_front();
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

SharedPtr<BlockNode> Parser::parse_case_when_body(LocalsHashmap &locals) {
    SharedPtr<BlockNode> body = new BlockNode { current_token() };
    validate_current_token();
    skip_newlines();
    while (!current_token().is_eof() && !current_token().is_when_keyword() && !current_token().is_else_keyword() && !current_token().is_end_keyword()) {
        auto exp = parse_expression(Precedence::LOWEST, locals);
        body->add_node(exp);
        validate_current_token();
        next_expression();
    }
    if (!current_token().is_when_keyword() && !current_token().is_else_keyword() && !current_token().is_end_keyword())
        throw_unexpected("case: when, else, or end");
    return body;
}

SharedPtr<Node> Parser::parse_class_or_module_name(LocalsHashmap &locals) {
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

SharedPtr<Node> Parser::parse_class(LocalsHashmap &locals) {
    auto token = current_token();
    if (peek_token().type() == Token::Type::LeftShift)
        return parse_sclass(locals);
    advance();
    LocalsHashmap our_locals { TM::HashType::TMString };
    SharedPtr<Node> name = parse_class_or_module_name(our_locals);
    SharedPtr<Node> superclass;
    if (current_token().type() == Token::Type::LessThan) {
        advance();
        superclass = parse_expression(Precedence::LOWEST, our_locals);
    } else {
        superclass = new NilNode { token };
    }
    SharedPtr<BlockNode> body = parse_body(our_locals, Precedence::LOWEST, Token::Type::EndKeyword, true);
    expect(Token::Type::EndKeyword, "class end");
    advance();
    return new ClassNode { token, name, superclass, body };
};

SharedPtr<Node> Parser::parse_multiple_assignment_expression(SharedPtr<Node> left, LocalsHashmap &locals) {
    if (!left->is_assignable())
        throw_unexpected("assignment =");
    SharedPtr<MultipleAssignmentNode> list = new MultipleAssignmentNode { left->token() };
    list->add_node(left);
    while (current_token().is_comma()) {
        advance();
        if (current_token().is_rparen() || current_token().is_equal()) {
            // trailing comma with no additional identifier
            break;
        }
        auto node = parse_assignment_identifier(true, locals);
        list->add_node(node);
    }
    return list.static_cast_as<Node>();
}

SharedPtr<Node> Parser::parse_assignment_identifier(bool allow_splat, LocalsHashmap &locals) {
    auto token = current_token();
    SharedPtr<Node> node;
    switch (token.type()) {
    case Token::Type::BareName:
    case Token::Type::ClassVariable:
    case Token::Type::Constant:
    case Token::Type::GlobalVariable:
    case Token::Type::InstanceVariable:
        node = parse_identifier(locals);
        break;
    case Token::Type::ConstantResolution:
        node = parse_top_level_constant(locals);
        break;
    case Token::Type::LParen: {
        advance(); // (
        node = parse_assignment_identifier(true, locals);
        node = parse_multiple_assignment_expression(node, locals);
        if (!current_token().is_rparen())
            throw_unexpected("closing paren for multiple assignment");
        advance(); // )
        break;
    }
    case Token::Type::Multiply: {
        if (!allow_splat)
            expect(Token::Type::BareName, "assignment identifier");
        auto splat_token = current_token();
        advance();
        if (current_token().is_assignable()) {
            auto id = parse_assignment_identifier(false, locals);
            node = new SplatNode { splat_token, id };
        } else {
            node = new SplatNode { splat_token };
        }
        break;
    }
    default:
        throw_unexpected("assignment identifier");
    }
    token = current_token();
    bool consumed = false;
    do {
        token = current_token();
        switch (token.type()) {
        case Token::Type::ConstantResolution:
            node = parse_constant_resolution_expression(node, locals);
            break;
        case Token::Type::Dot:
            node = parse_send_expression(node, locals);
            break;
        case Token::Type::LBracket:
            if (treat_left_bracket_as_element_reference(node, token))
                node = parse_ref_expression(node, locals);
            else
                consumed = true;
            break;
        default:
            consumed = true;
            break;
        }
    } while (!consumed);
    return node;
}

SharedPtr<Node> Parser::parse_constant(LocalsHashmap &) {
    auto node = new ConstantNode { current_token() };
    advance();
    return node;
};

SharedPtr<Node> Parser::parse_def(LocalsHashmap &locals) {
    auto def_token = current_token();
    advance();
    LocalsHashmap our_locals { TM::HashType::TMString };
    SharedPtr<Node> self_node;
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
    if (current_token().is_equal() && !current_token().whitespace_precedes()) {
        advance();
        name->append_char('=');
    }
    auto args = Vector<SharedPtr<Node>> {};
    if (current_token().is_lparen()) {
        advance();
        if (current_token().is_rparen()) {
            advance();
        } else {
            parse_def_args(args, our_locals);
            expect(Token::Type::RParen, "args closing paren");
            advance();
        }
    } else if (current_token().is_bare_name() || current_token().is_splat() || current_token().is_symbol_key()) {
        parse_def_args(args, our_locals);
    }
    SharedPtr<BlockNode> body;
    if (current_token().is_equal()) {
        advance(); // =
        auto exp = parse_expression(Precedence::LOWEST, our_locals);
        body = new BlockNode { exp->token(), exp };
    } else {
        body = parse_def_body(our_locals);
        expect(Token::Type::EndKeyword, "def end");
        advance();
    }
    return new DefNode {
        def_token,
        self_node,
        name,
        args,
        body
    };
};

SharedPtr<Node> Parser::parse_defined(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto arg = parse_expression(Precedence::BARE_CALL_ARG, locals);
    return new DefinedNode { token, arg };
}

void Parser::parse_def_args(Vector<SharedPtr<Node>> &args, LocalsHashmap &locals) {
    args.push(parse_def_single_arg(locals));
    while (current_token().is_comma()) {
        advance();
        args.push(parse_def_single_arg(locals));
    }
}

SharedPtr<Node> Parser::parse_def_single_arg(LocalsHashmap &locals) {
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::BareName: {
        SharedPtr<ArgNode> arg = new ArgNode { token, token.literal_string() };
        advance();
        arg->add_to_locals(locals);
        if (current_token().is_equal()) {
            advance();
            arg->set_value(parse_expression(Precedence::DEF_ARG, locals));
        }
        return arg.static_cast_as<Node>();
    }
    case Token::Type::LParen: {
        advance();
        auto sub_args = Vector<SharedPtr<Node>> {};
        parse_def_args(sub_args, locals);
        expect(Token::Type::RParen, "nested args closing paren");
        advance();
        auto masgn = new MultipleAssignmentArgNode { token };
        for (auto arg : sub_args) {
            masgn->add_node(arg);
        }
        return masgn;
    }
    case Token::Type::Multiply: {
        advance();
        SharedPtr<ArgNode> arg;
        if (current_token().is_bare_name()) {
            arg = new ArgNode { token, current_token().literal_string() };
            advance();
            arg->add_to_locals(locals);
        } else {
            arg = new ArgNode { token };
        }
        arg->set_splat(true);
        return arg.static_cast_as<Node>();
    }
    case Token::Type::Exponent: {
        advance();
        SharedPtr<ArgNode> arg;
        if (current_token().is_bare_name()) {
            arg = new ArgNode { token, current_token().literal_string() };
            advance();
            arg->add_to_locals(locals);
        } else {
            arg = new ArgNode { token };
        }
        arg->set_kwsplat(true);
        return arg.static_cast_as<Node>();
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
        SharedPtr<KeywordArgNode> arg = new KeywordArgNode { token, current_token().literal_string() };
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
        return arg.static_cast_as<Node>();
    }
    default:
        throw_unexpected("argument");
    }
}

SharedPtr<Node> Parser::parse_modifier_expression(SharedPtr<Node> left, LocalsHashmap &locals) {
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
        SharedPtr<BlockNode> body;
        bool pre = true;
        if (left->type() == Node::Type::Block) {
            body = left.static_cast_as<BlockNode>();
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
        SharedPtr<BlockNode> body;
        bool pre = true;
        if (left->type() == Node::Type::Block) {
            body = left.static_cast_as<BlockNode>();
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

SharedPtr<Node> Parser::parse_file_constant(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new StringNode { token, token.file() };
}

SharedPtr<Node> Parser::parse_group(LocalsHashmap &locals) {
    auto token = current_token();
    advance(); // (

    if (current_token().is_rparen()) {
        advance();
        return new NilSexpNode { token };
    }

    SharedPtr<Node> exp = parse_expression(Precedence::LOWEST, locals);

    if (current_token().is_end_of_expression()) {
        SharedPtr<BlockNode> block = new BlockNode { token };
        block->add_node(exp);
        while (current_token().is_end_of_expression()) {
            next_expression();
            auto next_exp = parse_expression(Precedence::LOWEST, locals);
            block->add_node(next_exp);
        }
        exp = block.static_cast_as<Node>();
    }
    expect(Token::Type::RParen, "group closing paren");
    advance(); // )

    if (current_token().is_equal() && exp->type() != Node::Type::MultipleAssignment) {
        throw_unexpected("multiple assignment on left-hand-side");
    }

    if (exp->type() == Node::Type::MultipleAssignment) {
        auto multiple_assignment_node = exp.static_cast_as<MultipleAssignmentNode>();
        if (multiple_assignment_node->increment_depth() > 1) {
            auto wrapper = new MultipleAssignmentNode { token };
            wrapper->increment_depth();
            wrapper->add_node(exp);
            exp = wrapper;
        }
    }

    return exp;
};

SharedPtr<Node> Parser::parse_hash(LocalsHashmap &locals) {
    expect(Token::Type::LCurlyBrace, "hash opening curly brace");
    auto token = current_token();
    advance();
    SharedPtr<Node> hash;
    if (current_token().type() == Token::Type::RCurlyBrace)
        hash = new HashNode { token };
    else
        hash = parse_hash_inner(locals, Precedence::HASH, Token::Type::RCurlyBrace);
    expect(Token::Type::RCurlyBrace, "hash closing curly brace");
    advance();
    return hash;
}

SharedPtr<Node> Parser::parse_hash_inner(LocalsHashmap &locals, Precedence precedence, Token::Type closing_token, SharedPtr<Node> first_key) {
    auto token = current_token();
    SharedPtr<HashNode> hash = new HashNode { token };
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
    return hash.static_cast_as<Node>();
}

SharedPtr<Node> Parser::parse_identifier(LocalsHashmap &locals) {
    auto name = current_token().literal();
    assert(name);
    bool is_lvar = !!locals.get(name);
    auto identifier = new IdentifierNode { current_token(), is_lvar };
    advance();
    return identifier;
};

SharedPtr<Node> Parser::parse_if(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    SharedPtr<Node> condition = parse_expression(Precedence::LOWEST, locals);
    if (current_token().type() == Token::Type::ThenKeyword) {
        advance(); // then
    } else {
        next_expression();
    }
    SharedPtr<Node> true_expr = parse_if_body(locals);
    SharedPtr<Node> false_expr;
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

SharedPtr<Node> Parser::parse_if_body(LocalsHashmap &locals) {
    SharedPtr<BlockNode> body = new BlockNode { current_token() };
    validate_current_token();
    skip_newlines();
    auto is_divider = [&]() {
        auto token = current_token();
        return token.is_elsif_keyword() || token.is_else_keyword() || token.is_end_keyword();
    };
    while (!current_token().is_eof() && !is_divider()) {
        auto exp = parse_expression(Precedence::LOWEST, locals);
        body->add_node(exp);
        validate_current_token();
        if (!is_divider())
            next_expression();
    }
    if (!is_divider())
        throw_unexpected("if end");
    if (body->is_empty())
        return new NilNode { body->token() };
    else if (body->has_one_node())
        return body->take_first_node();
    else
        return body.static_cast_as<Node>();
}

void Parser::parse_interpolated_body(LocalsHashmap &locals, InterpolatedNode &node, Token::Type end_token) {
    while (current_token().is_valid() && current_token().type() != end_token) {
        switch (current_token().type()) {
        case Token::Type::EvaluateToStringBegin: {
            advance();
            SharedPtr<BlockNode> block = new BlockNode { current_token() };
            while (current_token().type() != Token::Type::EvaluateToStringEnd) {
                block->add_node(parse_expression(Precedence::LOWEST, locals));
                skip_newlines();
            }
            advance();
            if (block->has_one_node()) {
                auto first = block->take_first_node();
                if (first->type() == Node::Type::String)
                    node.add_node(first);
                else
                    node.add_node(new EvaluateToStringNode { current_token(), first });
            } else {
                node.add_node(new EvaluateToStringNode { current_token(), block.static_cast_as<Node>() });
            }
            break;
        }
        case Token::Type::String:
            node.add_node(new StringNode { current_token(), current_token().literal_string() });
            advance();
            break;
        default:
            TM_UNREACHABLE();
        }
    }
    if (current_token().type() != end_token) {
        auto token = node.token();
        switch (current_token().type()) {
        case Token::Type::UnterminatedRegexp:
        case Token::Type::UnterminatedString:
        case Token::Type::UnterminatedWordArray:
            throw_unterminated_thing(token);
        default:
            // this shouldn't happen -- if it does, there is a bug in the Lexer
            TM_UNREACHABLE()
        }
    }
};

SharedPtr<Node> Parser::parse_interpolated_regexp(LocalsHashmap &locals) {
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
        SharedPtr<InterpolatedRegexpNode> interpolated_regexp = new InterpolatedRegexpNode { token };
        parse_interpolated_body(locals, interpolated_regexp.ref(), Token::Type::InterpolatedRegexpEnd);
        if (current_token().has_literal()) {
            auto str = current_token().literal_string().ref();
            int options_int = parse_regexp_options(str);
            interpolated_regexp->set_options(options_int);
        }
        advance();
        return interpolated_regexp.static_cast_as<Node>();
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

SharedPtr<Node> Parser::parse_interpolated_shell(LocalsHashmap &locals) {
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
        SharedPtr<InterpolatedNode> interpolated_shell = new InterpolatedShellNode { token };
        parse_interpolated_body(locals, interpolated_shell.ref(), Token::Type::InterpolatedShellEnd);
        advance();
        return interpolated_shell.static_cast_as<Node>();
    }
};

static SharedPtr<Node> convert_string_to_symbol_key(SharedPtr<Node> string) {
    switch (string->type()) {
    case Node::Type::String: {
        auto token = string->token();
        auto name = string.static_cast_as<StringNode>()->string();
        return new SymbolKeyNode { token, name };
    }
    case Node::Type::InterpolatedString: {
        auto token = string->token();
        auto node = new InterpolatedSymbolKeyNode { *string.static_cast_as<InterpolatedStringNode>() };
        return node;
    }
    default:
        TM_UNREACHABLE();
    }
}

SharedPtr<Node> Parser::parse_interpolated_string(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    SharedPtr<Node> string;
    if (current_token().type() == Token::Type::InterpolatedStringEnd) {
        string = new StringNode { token, new String };
        advance();
    } else if (current_token().type() == Token::Type::String && peek_token().type() == Token::Type::InterpolatedStringEnd) {
        string = new StringNode { token, current_token().literal_string() };
        advance();
        advance();
    } else {
        SharedPtr<InterpolatedNode> interpolated_string = new InterpolatedStringNode { token };
        parse_interpolated_body(locals, interpolated_string.ref(), Token::Type::InterpolatedStringEnd);
        advance();
        string = interpolated_string.static_cast_as<Node>();
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

SharedPtr<Node> Parser::parse_interpolated_symbol(LocalsHashmap &locals) {
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
        SharedPtr<InterpolatedNode> interpolated_symbol = new InterpolatedSymbolNode { token };
        parse_interpolated_body(locals, interpolated_symbol.ref(), Token::Type::InterpolatedSymbolEnd);
        advance();
        return interpolated_symbol.static_cast_as<Node>();
    }
};

SharedPtr<Node> Parser::parse_lit(LocalsHashmap &) {
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

SharedPtr<Node> Parser::parse_keyword_splat(LocalsHashmap &locals) {
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

SharedPtr<Node> Parser::parse_module(LocalsHashmap &) {
    auto token = current_token();
    advance();
    LocalsHashmap our_locals { TM::HashType::TMString };
    SharedPtr<Node> name = parse_class_or_module_name(our_locals);
    SharedPtr<BlockNode> body = parse_body(our_locals, Precedence::LOWEST, Token::Type::EndKeyword, true);
    expect(Token::Type::EndKeyword, "module end");
    advance();
    return new ModuleNode { token, name, body };
}

SharedPtr<Node> Parser::parse_next(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    if (current_token().is_lparen()) {
        advance();
        if (current_token().is_rparen()) {
            advance();
            return new NextNode { token, new NilSexpNode { token } };
        } else {
            SharedPtr<Node> arg = parse_expression(Precedence::BARE_CALL_ARG, locals);
            expect(Token::Type::RParen, "break closing paren");
            advance();
            return new NextNode { token, arg };
        }
    } else if (current_token().can_be_first_arg_of_implicit_call()) {
        auto value = parse_expression(Precedence::BARE_CALL_ARG, locals);
        if (current_token().is_comma()) {
            SharedPtr<ArrayNode> array = new ArrayNode { token };
            array->add_node(value);
            while (current_token().is_comma()) {
                advance();
                array->add_node(parse_expression(Precedence::BARE_CALL_ARG, locals));
            }
            value = array.static_cast_as<Node>();
        }
        return new NextNode { token, value };
    }
    return new NextNode { token };
}

SharedPtr<Node> Parser::parse_nil(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new NilSexpNode { token };
}

SharedPtr<Node> Parser::parse_not(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto precedence = get_precedence(token);
    auto node = new NotNode {
        token,
        parse_expression(precedence, locals),
    };
    return node;
}

SharedPtr<Node> Parser::parse_nth_ref(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new NthRefNode { token, token.get_fixnum() };
}

SharedPtr<Node> Parser::parse_redo(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new RedoNode { token };
}

SharedPtr<Node> Parser::parse_retry(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new RetryNode { token };
}

SharedPtr<Node> Parser::parse_return(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    SharedPtr<Node> value;
    if (current_token().is_end_of_expression()) {
        value = new NilNode { token };
    } else {
        value = parse_expression(Precedence::BARE_CALL_ARG, locals);
    }
    if (current_token().is_comma()) {
        SharedPtr<ArrayNode> array = new ArrayNode { current_token() };
        array->add_node(value);
        while (current_token().is_comma()) {
            advance();
            array->add_node(parse_expression(Precedence::BARE_CALL_ARG, locals));
        }
        value = array.static_cast_as<Node>();
    }
    return new ReturnNode { token, value };
};

SharedPtr<Node> Parser::parse_sclass(LocalsHashmap &locals) {
    auto token = current_token();
    advance(); // class
    advance(); // <<
    SharedPtr<Node> klass = parse_expression(Precedence::BARE_CALL_ARG, locals);
    SharedPtr<BlockNode> body = parse_body(locals, Precedence::LOWEST);
    expect(Token::Type::EndKeyword, "sclass end");
    advance();
    return new SclassNode { token, klass, body };
}

SharedPtr<Node> Parser::parse_self(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new SelfNode { token };
};

SharedPtr<Node> Parser::parse_splat(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    return new SplatNode { token, parse_expression(Precedence::SPLAT, locals) };
};

SharedPtr<Node> Parser::parse_stabby_proc(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    bool has_args = false;
    auto args = Vector<SharedPtr<Node>> {};
    if (current_token().is_lparen()) {
        has_args = true;
        advance();
        if (current_token().is_rparen()) {
            advance();
        } else {
            parse_def_args(args, locals);
            expect(Token::Type::RParen, "proc args closing paren");
            advance();
        }
    } else if (current_token().is_bare_name() || current_token().type() == Token::Type::Multiply) {
        has_args = true;
        parse_def_args(args, locals);
    }
    if (current_token().type() != Token::Type::DoKeyword && current_token().type() != Token::Type::LCurlyBrace)
        throw_unexpected("block");
    return new StabbyProcNode {
        token,
        has_args,
        args
    };
};

SharedPtr<Node> Parser::parse_string(LocalsHashmap &locals) {
    auto string_token = current_token();
    SharedPtr<Node> string = new StringNode { string_token, string_token.literal_string() };
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

SharedPtr<Node> Parser::concat_adjacent_strings(SharedPtr<Node> string, LocalsHashmap &locals, bool &strings_were_appended) {
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

SharedPtr<Node> Parser::append_string_nodes(SharedPtr<Node> string1, SharedPtr<Node> string2) {
    switch (string1->type()) {
    case Node::Type::String: {
        auto string1_node = string1.static_cast_as<StringNode>();
        switch (string2->type()) {
        case Node::Type::String: {
            auto string2_node = string2.static_cast_as<StringNode>();
            string1_node->string()->append(*string2_node->string());
            return string1;
        }
        case Node::Type::InterpolatedString: {
            auto string2_node = string2.static_cast_as<InterpolatedStringNode>();
            assert(!string2_node->is_empty());
            if (string2_node->nodes().first()->type() == Node::Type::String) {
                auto n1 = string2_node->nodes().first().static_cast_as<StringNode>();
                n1->string()->prepend(*string1_node->string());
            } else {
                string2_node->prepend_node(string1_node.static_cast_as<Node>());
            }
            return string2;
        }
        default:
            TM_UNREACHABLE();
        }
        break;
    }
    case Node::Type::InterpolatedString: {
        auto string1_node = string1.static_cast_as<InterpolatedStringNode>();
        switch (string2->type()) {
        case Node::Type::String: {
            auto string2_node = string2.static_cast_as<StringNode>();
            assert(!string1_node->is_empty());
            auto last_node = string1_node->nodes().last();
            switch (last_node->type()) {
            case Node::Type::String:
                if (string1_node->nodes().size() > 1) {
                    // For some reason, RubyParser doesn't append two string nodes
                    // if there is an evstr present.
                    string1_node->add_node(string2);
                } else {
                    last_node.static_cast_as<StringNode>()->string()->append(*string2_node->string());
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
            auto string2_node = string2.static_cast_as<InterpolatedStringNode>();
            assert(!string1_node->is_empty());
            assert(!string2_node->is_empty());
            for (auto node : string2_node->nodes()) {
                string1_node->add_node(node);
            }
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

SharedPtr<Node> Parser::parse_super(LocalsHashmap &) {
    auto token = current_token();
    advance();
    auto node = new SuperNode { token };
    if (current_token().is_lparen())
        node->set_parens(true);
    return node;
};

SharedPtr<Node> Parser::parse_symbol(LocalsHashmap &) {
    auto token = current_token();
    auto symbol = new SymbolNode { token, current_token().literal_string() };
    advance();
    return symbol;
};

SharedPtr<Node> Parser::parse_symbol_key(LocalsHashmap &) {
    auto token = current_token();
    auto symbol = new SymbolKeyNode { token, current_token().literal_string() };
    advance();
    return symbol;
};

SharedPtr<Node> Parser::parse_top_level_constant(LocalsHashmap &) {
    auto token = current_token();
    advance();
    auto name_token = current_token();
    SharedPtr<String> name;
    switch (name_token.type()) {
    case Token::Type::BareName:
    case Token::Type::Constant:
        advance();
        name = name_token.literal_string();
        break;
    default:
        throw_unexpected(name_token, ":: identifier name");
    }
    return new Colon3Node { token, name };
}

SharedPtr<Node> Parser::parse_unary_operator(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto precedence = get_precedence(token);
    auto receiver = parse_expression(precedence, locals);
    if ((token.type() == Token::Type::Minus || token.type() == Token::Type::Plus) && receiver->is_numeric()) {
        switch (receiver->type()) {
        case Node::Type::Bignum: {
            if (token.type() == Token::Type::Minus) {
                auto num = receiver.static_cast_as<BignumNode>();
                num->negate();
                return num.static_cast_as<Node>();
            }
            return receiver;
        }
        case Node::Type::Fixnum: {
            if (token.type() == Token::Type::Minus) {
                auto num = receiver.static_cast_as<FixnumNode>();
                num->negate();
                return num.static_cast_as<Node>();
            }
            return receiver;
        }
        case Node::Type::Float: {
            if (token.type() == Token::Type::Minus) {
                auto num = receiver.static_cast_as<FloatNode>();
                num->negate();
                return num.static_cast_as<Node>();
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

SharedPtr<Node> Parser::parse_undef(LocalsHashmap &locals) {
    auto undef_token = current_token();
    advance();
    auto symbol_from_token = [&](Token &token) -> SharedPtr<Node> {
        switch (token.type()) {
        case Token::Type::BareName:
        case Token::Type::Constant:
            advance();
            return new SymbolNode { token, token.literal_string() };
        case Token::Type::Symbol:
            return parse_symbol(locals);
        case Token::Type::InterpolatedSymbolBegin: {
            return parse_interpolated_symbol(locals);
        }
        default:
            throw_unexpected("method name for undef");
        }
    };
    SharedPtr<UndefNode> undef_node = new UndefNode { undef_token };
    auto token = current_token();
    undef_node->add_arg(symbol_from_token(token));
    if (current_token().is_comma()) {
        SharedPtr<BlockNode> block = new BlockNode { undef_token };
        block->add_node(undef_node.static_cast_as<Node>());
        while (current_token().is_comma()) {
            advance();
            token = current_token();
            SharedPtr<UndefNode> undef_node = new UndefNode { undef_token };
            undef_node->add_arg(symbol_from_token(token));
            block->add_node(undef_node.static_cast_as<Node>());
        }
        return block.static_cast_as<Node>();
    }
    return undef_node.static_cast_as<Node>();
};

SharedPtr<Node> Parser::parse_word_array(LocalsHashmap &locals) {
    auto token = current_token();
    SharedPtr<ArrayNode> array = new ArrayNode { token };
    advance();
    while (!current_token().is_eof() && current_token().type() != Token::Type::RBracket) {
        if (current_token().type() == Token::Type::UnterminatedWordArray)
            throw_unterminated_thing(current_token(), token);
        array->add_node(parse_expression(Precedence::WORD_ARRAY, locals));
    }
    expect(Token::Type::RBracket, "closing array bracket");
    advance();
    return array.static_cast_as<Node>();
}

SharedPtr<Node> Parser::parse_word_symbol_array(LocalsHashmap &locals) {
    auto token = current_token();
    SharedPtr<ArrayNode> array = new ArrayNode { token };
    advance();
    while (!current_token().is_eof() && current_token().type() != Token::Type::RBracket) {
        auto string = parse_expression(Precedence::WORD_ARRAY, locals);
        SharedPtr<Node> symbol_node;
        switch (string->type()) {
        case Node::Type::String:
            symbol_node = string.static_cast_as<StringNode>()->to_symbol_node().static_cast_as<Node>();
            break;
        case Node::Type::InterpolatedString:
            symbol_node = string.static_cast_as<InterpolatedStringNode>()->to_symbol_node().static_cast_as<Node>();
            break;
        default:
            TM_UNREACHABLE();
        }
        array->add_node(symbol_node);
    }
    expect(Token::Type::RBracket, "closing array bracket");
    advance();
    return array.static_cast_as<Node>();
}

SharedPtr<Node> Parser::parse_yield(LocalsHashmap &) {
    auto token = current_token();
    advance();
    return new YieldNode { token };
};

SharedPtr<Node> Parser::parse_assignment_expression(SharedPtr<Node> left, LocalsHashmap &locals) {
    return parse_assignment_expression(left, locals, true);
}

SharedPtr<Node> Parser::parse_assignment_expression_without_multiple_values(SharedPtr<Node> left, LocalsHashmap &locals) {
    return parse_assignment_expression(left, locals, false);
}

SharedPtr<Node> Parser::parse_assignment_expression(SharedPtr<Node> left, LocalsHashmap &locals, bool allow_multiple) {
    auto token = current_token();
    if (left->type() == Node::Type::Splat) {
        return parse_multiple_assignment_expression(left, locals);
    }
    switch (left->type()) {
    case Node::Type::Identifier: {
        auto left_identifier = left.static_cast_as<IdentifierNode>();
        left_identifier->add_to_locals(locals);
        advance();
        auto value = parse_assignment_expression_value(false, locals, allow_multiple);
        return new AssignmentNode { token, left, value };
    }
    case Node::Type::Call:
    case Node::Type::Colon2:
    case Node::Type::Colon3:
    case Node::Type::SafeCall: {
        advance();
        auto value = parse_assignment_expression_value(false, locals, allow_multiple);
        return new AssignmentNode { token, left, value };
    }
    case Node::Type::MultipleAssignment: {
        left.static_cast_as<MultipleAssignmentNode>()->add_locals(locals);
        advance();
        auto value = parse_assignment_expression_value(true, locals, allow_multiple);
        return new AssignmentNode { token, left, value };
    }
    default:
        throw_unexpected(left->token(), "left side of assignment");
    }
};

SharedPtr<Node> Parser::parse_assignment_expression_value(bool to_array, LocalsHashmap &locals, bool allow_multiple) {
    auto token = current_token();
    auto value = parse_expression(Precedence::ASSIGNMENT_RHS, locals);
    bool is_splat;

    if (allow_multiple && current_token().type() == Token::Type::Comma) {
        SharedPtr<ArrayNode> array = new ArrayNode { token };
        array->add_node(value);
        while (current_token().type() == Token::Type::Comma) {
            advance();
            array->add_node(parse_expression(Precedence::ASSIGNMENT_RHS, locals));
        }
        value = array.static_cast_as<Node>();
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

SharedPtr<Node> Parser::parse_iter_expression(SharedPtr<Node> left, LocalsHashmap &locals) {
    auto token = current_token();
    LocalsHashmap our_locals { locals }; // copy!
    bool curly_brace = current_token().type() == Token::Type::LCurlyBrace;
    bool has_args = false;
    auto args = Vector<SharedPtr<Node>> {};
    if (left->type() == Node::Type::StabbyProc) {
        advance();
        auto stabby_proc_node = left.static_cast_as<StabbyProcNode>();
        has_args = stabby_proc_node->has_args();
        for (auto arg : stabby_proc_node->args())
            args.push(arg);
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
    SharedPtr<BlockNode> body = parse_iter_body(our_locals, curly_brace);
    auto end_token_type = curly_brace ? Token::Type::RCurlyBrace : Token::Type::EndKeyword;
    expect(end_token_type, curly_brace ? "}" : "end");
    advance();
    return new IterNode {
        token,
        left,
        has_args,
        args,
        body
    };
}

void Parser::parse_iter_args(Vector<SharedPtr<Node>> &args, LocalsHashmap &locals) {
    args.push(parse_def_single_arg(locals));
    while (current_token().is_comma()) {
        advance();
        if (current_token().is_block_arg_delimiter()) {
            // trailing comma with no additional arg
            args.push(new NilNode { current_token() });
            break;
        }
        args.push(parse_def_single_arg(locals));
    }
}

SharedPtr<BlockNode> Parser::parse_iter_body(LocalsHashmap &locals, bool curly_brace) {
    auto end_token_type = curly_brace ? Token::Type::RCurlyBrace : Token::Type::EndKeyword;
    return parse_body(locals, Precedence::LOWEST, end_token_type, true);
}

SharedPtr<Node> Parser::parse_call_expression_with_parens(SharedPtr<Node> left, LocalsHashmap &locals) {
    auto token = current_token(); // (
    SharedPtr<NodeWithArgs> call_node = to_node_with_args(left);
    advance();
    if (current_token().is_rparen()) {
        // foo () vs foo()
        if (token.whitespace_precedes())
            call_node->add_arg(new NilSexpNode { current_token() });
    } else {
        parse_call_args(call_node.ref(), locals, false);
    }
    expect(Token::Type::RParen, "call rparen");
    advance();
    return call_node.static_cast_as<Node>();
}

SharedPtr<NodeWithArgs> Parser::to_node_with_args(SharedPtr<Node> node) {
    switch (node->type()) {
    case Node::Type::Identifier: {
        auto identifier = node.static_cast_as<IdentifierNode>();
        auto call_node = new CallNode {
            identifier->token(),
            new NilNode { identifier->token() },
            identifier->name(),
        };
        return call_node;
    }
    case Node::Type::Call:
    case Node::Type::SafeCall:
    case Node::Type::Super:
    case Node::Type::Undef:
    case Node::Type::Yield:
        return node.static_cast_as<NodeWithArgs>();
    default:
        throw_unexpected(current_token(), nullptr, "left-hand-side is not callable");
    }
}

void Parser::parse_call_args(NodeWithArgs &node, LocalsHashmap &locals, bool bare) {
    auto arg = parse_expression(bare ? Precedence::BARE_CALL_ARG : Precedence::CALL_ARG, locals);
    if (current_token().type() == Token::Type::HashRocket || arg->is_symbol_key()) {
        node.add_arg(parse_call_hash_args(locals, bare, arg));
    } else {
        node.add_arg(arg);
        while (current_token().is_comma()) {
            advance();
            auto token = current_token();
            if (token.is_rparen()) {
                // trailing comma with no additional arg
                break;
            }
            arg = parse_expression(bare ? Precedence::BARE_CALL_ARG : Precedence::CALL_ARG, locals);
            if (current_token().type() == Token::Type::HashRocket || arg->is_symbol_key()) {
                node.add_arg(parse_call_hash_args(locals, bare, arg));
                break;
            } else {
                node.add_arg(arg);
            }
        }
    }
}

SharedPtr<Node> Parser::parse_call_hash_args(LocalsHashmap &locals, bool bare, SharedPtr<Node> first_arg) {
    if (bare)
        return parse_hash_inner(locals, Precedence::BARE_CALL_ARG, Token::Type::Invalid, first_arg);
    else
        return parse_hash_inner(locals, Precedence::CALL_ARG, Token::Type::RParen, first_arg);
}

SharedPtr<Node> Parser::parse_call_expression_without_parens(SharedPtr<Node> left, LocalsHashmap &locals) {
    auto token = current_token();
    SharedPtr<NodeWithArgs> call_node = to_node_with_args(left);
    switch (token.type()) {
    case Token::Type::Comma:
    case Token::Type::Eof:
    case Token::Type::Eol:
    case Token::Type::RBracket:
    case Token::Type::RCurlyBrace:
    case Token::Type::RParen:
        break;
    default:
        parse_call_args(call_node.ref(), locals, true);
    }
    return call_node.static_cast_as<Node>();
}

SharedPtr<Node> Parser::parse_constant_resolution_expression(SharedPtr<Node> left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto name_token = current_token();
    SharedPtr<Node> node;
    switch (name_token.type()) {
    case Token::Type::BareName:
        advance();
        node = new CallNode { name_token, left, name_token.literal_string() };
        break;
    case Token::Type::Constant:
        advance();
        node = new Colon2Node { name_token, left, name_token.literal_string() };
        break;
    case Token::Type::LParen: {
        advance();
        SharedPtr<CallNode> call_node = new CallNode { name_token, left, new String("call") };
        if (!current_token().is_rparen())
            parse_call_args(call_node.ref(), locals, false);
        expect(Token::Type::RParen, "::() call right paren");
        advance();
        node = call_node.static_cast_as<Node>();
        break;
    }
    default:
        throw_unexpected(name_token, ":: identifier name");
    }
    return node;
}

SharedPtr<Node> Parser::parse_infix_expression(SharedPtr<Node> left, LocalsHashmap &locals) {
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

SharedPtr<Node> Parser::parse_logical_expression(SharedPtr<Node> left, LocalsHashmap &locals) {
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

SharedPtr<Node> Parser::parse_match_expression(SharedPtr<Node> left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    auto arg = parse_expression(Precedence::EQUALITY, locals);
    if (left->type() == Node::Type::Regexp) {
        return new MatchNode { token, left.static_cast_as<RegexpNode>(), arg, true };
    } else if (arg->type() == Node::Type::Regexp) {
        return new MatchNode { token, arg.static_cast_as<RegexpNode>(), left, false };
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

SharedPtr<Node> Parser::parse_not_match_expression(SharedPtr<Node> left, LocalsHashmap &locals) {
    auto token = current_token();
    left = parse_match_expression(left, locals);
    return new NotMatchNode { token, left };
}

SharedPtr<Node> Parser::parse_op_assign_expression(SharedPtr<Node> left, LocalsHashmap &locals) {
    if (left->type() == Node::Type::Call)
        return parse_op_attr_assign_expression(left, locals);
    switch (left->type()) {
    case Node::Type::Identifier: {
        auto identifier = left.static_cast_as<IdentifierNode>();
        identifier->set_is_lvar(true);
        identifier->add_to_locals(locals);
        break;
    }
    case Node::Type::Constant:
    case Node::Type::Colon2:
    case Node::Type::Colon3:
        break;
    default:
        throw_unexpected(left->token(), "variable or constant");
    }
    auto token = current_token();
    advance();
    switch (token.type()) {
    case Token::Type::AndEqual:
        return new OpAssignAndNode { token, left, parse_expression(Precedence::ASSIGNMENT_RHS, locals) };
    case Token::Type::OrEqual:
        return new OpAssignOrNode { token, left, parse_expression(Precedence::ASSIGNMENT_RHS, locals) };
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
        return new OpAssignNode { token, op, left, parse_expression(Precedence::ASSIGNMENT_RHS, locals) };
    }
    default:
        TM_UNREACHABLE();
    }
}

SharedPtr<Node> Parser::parse_op_attr_assign_expression(SharedPtr<Node> left, LocalsHashmap &locals) {
    if (left->type() != Node::Type::Call)
        throw_unexpected(left->token(), "call");
    auto left_call = left.static_cast_as<CallNode>();
    auto token = current_token();
    advance();
    auto value = parse_expression(Precedence::OP_ASSIGNMENT, locals);
    switch (token.type()) {
    case Token::Type::AndEqual:
        return new OpAssignAndNode {
            token,
            left,
            value,
        };
    case Token::Type::OrEqual:
        return new OpAssignOrNode {
            token,
            left,
            value,
        };
    default: {
        auto op = new String(token.type_value());
        op->chomp();
        SharedPtr<String> message = new String(left_call->message().ref());
        message->append_char('=');
        return new OpAssignAccessorNode {
            token,
            op,
            left_call->receiver(),
            message,
            value,
            left_call->args(),
        };
    }
    }
}

SharedPtr<Node> Parser::parse_proc_call_expression(SharedPtr<Node> left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    advance();
    SharedPtr<CallNode> call_node = new CallNode {
        token,
        left,
        new String("call"),
    };
    if (!current_token().is_rparen())
        parse_call_args(call_node.ref(), locals, false);
    expect(Token::Type::RParen, "proc call right paren");
    advance();
    return call_node.static_cast_as<Node>();
}

SharedPtr<Node> Parser::parse_range_expression(SharedPtr<Node> left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    SharedPtr<Node> right;
    try {
        right = parse_expression(Precedence::RANGE, locals);
    } catch (SyntaxError &e) {
        // NOTE: I'm not sure if this is the "right" way to handle an endless range,
        // but it seems to be effective for the tests I threw at it. ¯\_(ツ)_/¯
        right = new NilNode { token };
    }
    return new RangeNode { token, left, right, token.type() == Token::Type::DotDotDot };
}

SharedPtr<Node> Parser::parse_ref_expression(SharedPtr<Node> left, LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    SharedPtr<CallNode> call_node = new CallNode {
        token,
        left,
        new String("[]"),
    };
    if (token.type() == Token::Type::LBracketRBracket) {
        return call_node.static_cast_as<Node>();
    }
    if (current_token().type() != Token::Type::RBracket)
        parse_call_args(call_node.ref(), locals, false);
    expect(Token::Type::RBracket, "element reference right bracket");
    advance();
    return call_node.static_cast_as<Node>();
}

SharedPtr<Node> Parser::parse_rescue_expression(SharedPtr<Node> left, LocalsHashmap &locals) {
    auto token = current_token();
    advance(); // rescue
    auto value = parse_expression(Precedence::LOWEST, locals);
    auto body = new BlockNode { left->token(), left };
    auto begin_node = new BeginNode { token, body };
    auto rescue_node = new BeginRescueNode { token };
    rescue_node->set_body(new BlockNode { value->token(), value });
    begin_node->add_rescue_node(rescue_node);
    return begin_node;
}

SharedPtr<Node> Parser::parse_safe_send_expression(SharedPtr<Node> left, LocalsHashmap &) {
    auto token = current_token();
    advance();
    auto name_token = current_token();
    SharedPtr<String> name;
    switch (name_token.type()) {
    case Token::Type::BareName:
    case Token::Type::Constant:
        name = name_token.literal_string();
        advance();
        break;
    default:
        throw_unexpected("safe navigation method name");
    }
    return new SafeCallNode {
        token,
        left,
        name_token.literal_string(),
    };
}

SharedPtr<Node> Parser::parse_send_expression(SharedPtr<Node> left, LocalsHashmap &) {
    auto dot_token = current_token();
    advance();
    auto name_token = current_token();
    SharedPtr<String> name = new String();
    switch (name_token.type()) {
    case Token::Type::BareName:
    case Token::Type::Constant:
        name = name_token.literal_string();
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

SharedPtr<Node> Parser::parse_ternary_expression(SharedPtr<Node> left, LocalsHashmap &locals) {
    auto token = current_token();
    expect(Token::Type::TernaryQuestion, "ternary question");
    advance();
    SharedPtr<Node> true_expr = parse_expression(Precedence::TERNARY_TRUE, locals);
    expect(Token::Type::TernaryColon, "ternary colon");
    advance();
    auto false_expr = parse_expression(Precedence::TERNARY_FALSE, locals);
    return new IfNode { token, left, true_expr, false_expr };
}

SharedPtr<Node> Parser::parse_unless(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    SharedPtr<Node> condition = parse_expression(Precedence::LOWEST, locals);
    next_expression();
    SharedPtr<Node> false_expr = parse_if_body(locals);
    SharedPtr<Node> true_expr;
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

SharedPtr<Node> Parser::parse_while(LocalsHashmap &locals) {
    auto token = current_token();
    advance();
    SharedPtr<Node> condition = parse_expression(Precedence::LOWEST, locals);
    next_expression();
    SharedPtr<BlockNode> body = parse_body(locals, Precedence::LOWEST);
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
        return {};
    }
}

Parser::parse_left_fn Parser::left_denotation(Token &token, SharedPtr<Node> left, Precedence precedence) {
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
    default:
        break;
    }
    if (is_first_arg_of_call_without_parens(left, token))
        return &Parser::parse_call_expression_without_parens;
    return {};
}

bool Parser::is_first_arg_of_call_without_parens(SharedPtr<Node> left, Token &token) {
    return left->is_callable() && token.can_be_first_arg_of_implicit_call();
}

Token &Parser::current_token() const {
    if (m_index < m_tokens->size()) {
        return m_tokens->at(m_index);
    } else {
        return Token::invalid();
    }
}

Token &Parser::peek_token() const {
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

void Parser::throw_unexpected(const Token &token, const char *expected, const char *error) {
    auto file = token.file() ? String(*token.file()) : String("(unknown)");
    auto line = token.line() + 1;
    auto type = token.type_value();
    auto literal = token.literal();
    const char *help = nullptr;
    const char *help_description = nullptr;
    if (error) {
        assert(!expected);
        help = error;
        help_description = "error";
    } else {
        assert(expected);
        help = expected;
        help_description = "expected";
    }
    String message;
    if (token.type() == Token::Type::Invalid) {
        message = String::format("{}#{}: syntax error, unexpected '{}' ({}: '{}')", file, line, token.literal(), help_description, help);
    } else if (!type) {
        message = String::format("{}#{}: syntax error, {} '{}' (token type: {})", file, line, help_description, help, (long long)token.type());
    } else if (token.type() == Token::Type::Eof) {
        auto indent = String { token.column(), ' ' };
        message = String::format(
            "{}#{}: syntax error, unexpected end-of-input ({}: '{}')\n"
            "{}\n"
            "{}^ here, {} '{}'",
            file, line, help_description, help, current_line(), indent, help_description, help);
    } else if (literal) {
        auto indent = String { token.column(), ' ' };
        message = String::format(
            "{}#{}: syntax error, unexpected {} '{}' ({}: '{}')\n"
            "{}\n"
            "{}^ here, {} '{}'",
            file, line, type, literal, help_description, help, current_line(), indent, help_description, help);
    } else {
        auto indent = String { token.column(), ' ' };
        message = String::format(
            "{}#{}: syntax error, unexpected '{}' ({}: '{}')\n"
            "{}\n"
            "{}^ here, {} '{}'",
            file, line, type, help_description, help, current_line(), indent, help_description, help);
    }
    throw SyntaxError { message };
}

void Parser::throw_unexpected(const char *expected) {
    throw_unexpected(current_token(), expected);
}

void Parser::throw_unterminated_thing(Token token, Token start_token) {
    if (!start_token) start_token = token;
    auto indent = String { start_token.column(), ' ' };
    String expected;
    const char *lit = start_token.literal();
    assert(lit);
    if (strcmp(lit, "(") == 0)
        expected = "')'";
    else if (strcmp(lit, "[") == 0)
        expected = "']'";
    else if (strcmp(lit, "{") == 0)
        expected = "'}'";
    else if (strcmp(lit, "<") == 0)
        expected = "'>'";
    else if (strcmp(lit, "'") == 0)
        expected = "\"'\"";
    else
        expected = String::format("'{}'", lit);
    assert(!expected.is_empty());
    const char *thing = nullptr;
    switch (token.type()) {
    case Token::Type::InterpolatedRegexpBegin:
    case Token::Type::UnterminatedRegexp:
        thing = "regexp";
        break;
    case Token::Type::InterpolatedShellBegin:
        thing = "shell";
        break;
    case Token::Type::InterpolatedStringBegin:
    case Token::Type::String:
    case Token::Type::UnterminatedString:
        thing = "string";
        break;
    case Token::Type::InterpolatedSymbolBegin:
        thing = "symbol";
        break;
    case Token::Type::UnterminatedWordArray:
        thing = "word array";
        break;
    default:
        printf("unhandled unterminated thing (token type = %d)\n", (int)token.type());
        TM_UNREACHABLE();
    }
    auto file = start_token.file() ? String(*start_token.file()) : String("(unknown)");
    auto line = start_token.line() + 1;
    auto code = code_line(start_token.line());
    auto message = String::format(
        "{}#{}: syntax error, unterminated {} meets end of file (expected: {})\n"
        "{}\n"
        "{}^ starts here, expected closing {} somewhere after",
        file, line, thing, expected, code, indent, expected);
    throw SyntaxError { message };
}

String Parser::code_line(size_t number) {
    size_t line = 0;
    String buf;
    for (size_t i = 0; i < m_code->size(); ++i) {
        char c = (*m_code)[i];
        if (line == number && c != '\n')
            buf.append_char(c);
        else if (line > number)
            break;
        if (c == '\n')
            line++;
    }
    return buf;
}

String Parser::current_line() {
    return code_line(current_token().line());
}

void Parser::validate_current_token() {
    auto token = current_token();
    switch (token.type()) {
    case Token::Type::Invalid:
        throw Parser::SyntaxError { String::format("{}: syntax error, unexpected '{}'", token.line() + 1, token.literal_or_blank()) };
    case Token::Type::InvalidUnicodeEscape:
        throw Parser::SyntaxError { String::format("{}: invalid Unicode escape", token.line() + 1) };
    case Token::Type::InvalidCharacterEscape:
        throw Parser::SyntaxError { String::format("{}: invalid character escape", token.line() + 1) };
    case Token::Type::UnterminatedRegexp:
    case Token::Type::UnterminatedString:
    case Token::Type::UnterminatedWordArray: {
        throw_unterminated_thing(current_token());
    }
    default:
        assert(token.type_value()); // all other types should return a string for type_value()
        return;
    }
}

}
