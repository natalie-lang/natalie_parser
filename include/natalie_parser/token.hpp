#pragma once

#include "tm/macros.hpp"
#include "tm/optional.hpp"
#include "tm/shared_ptr.hpp"
#include "tm/string.hpp"

namespace NatalieParser {

using namespace TM;

class Token {
public:
    enum class Type {
        Invalid, // must be first
        InvalidCharacterEscape,
        InvalidUnicodeEscape,
        AliasKeyword,
        And,
        AndEqual,
        AndKeyword,
        Arrow,
        BackRef,
        BareName,
        BeginKeyword,
        BEGINKeyword,
        Bignum,
        BitwiseAnd,
        BitwiseAndEqual,
        BitwiseOr,
        BitwiseOrEqual,
        BitwiseXor,
        BitwiseXorEqual,
        BreakKeyword,
        CaseKeyword,
        ClassKeyword,
        ClassVariable,
        Comma,
        Comment,
        Comparison,
        Constant,
        ConstantResolution,
        DefinedKeyword,
        DefKeyword,
        Divide,
        DivideEqual,
        Doc,
        DoKeyword,
        Dot,
        DotDot,
        DotDotDot,
        ElseKeyword,
        ElsifKeyword,
        ENCODINGKeyword,
        EndKeyword,
        ENDKeyword,
        EnsureKeyword,
        Eof,
        Eol,
        Equal,
        EqualEqual,
        EqualEqualEqual,
        EvaluateToStringBegin,
        EvaluateToStringEnd,
        Exponent,
        ExponentEqual,
        FalseKeyword,
        FILEKeyword,
        Fixnum,
        Float,
        ForKeyword,
        GlobalVariable,
        GreaterThan,
        GreaterThanOrEqual,
        HashRocket,
        IfKeyword,
        InKeyword,
        InstanceVariable,
        InterpolatedHeredocBegin,
        InterpolatedHeredocEnd,
        InterpolatedRegexpBegin,
        InterpolatedRegexpEnd,
        InterpolatedShellBegin,
        InterpolatedShellEnd,
        InterpolatedStringBegin,
        InterpolatedStringEnd,
        InterpolatedSymbolBegin,
        InterpolatedSymbolEnd,
        LCurlyBrace,
        LBracket,
        LBracketRBracket,
        LBracketRBracketEqual,
        LeftShift,
        LeftShiftEqual,
        LessThan,
        LessThanOrEqual,
        LINEKeyword,
        LParen,
        Match,
        Minus,
        MinusEqual,
        ModuleKeyword,
        Modulus,
        ModulusEqual,
        Multiply,
        MultiplyEqual,
        NextKeyword,
        NilKeyword,
        Not,
        NotEqual,
        NotKeyword,
        NotMatch,
        NthRef,
        Or,
        OrEqual,
        OrKeyword,
        PercentLowerI,
        PercentLowerW,
        PercentUpperI,
        PercentUpperW,
        Plus,
        PlusEqual,
        RCurlyBrace,
        RBracket,
        RedoKeyword,
        RescueKeyword,
        RetryKeyword,
        ReturnKeyword,
        RightShift,
        RightShiftEqual,
        RParen,
        SafeNavigation,
        SelfKeyword,
        Semicolon,
        String,
        SuperKeyword,
        Symbol,
        SymbolKey,
        TernaryColon,
        TernaryQuestion,
        ThenKeyword,
        Tilde,
        TrueKeyword,
        UndefKeyword,
        UnlessKeyword,
        UnterminatedRegexp,
        UnterminatedString,
        UntilKeyword,
        WhenKeyword,
        WhileKeyword,
        YieldKeyword,
    };

    Token() { }

    Token(Type type, SharedPtr<String> file, size_t line, size_t column)
        : m_type { type }
        , m_file { file }
        , m_line { line }
        , m_column { column } {
        assert(file);
    }

    Token(Type type, const char *literal, SharedPtr<String> file, size_t line, size_t column)
        : m_type { type }
        , m_literal { new String(literal) }
        , m_file { file }
        , m_line { line }
        , m_column { column } {
        assert(literal);
        assert(file);
    }

    Token(Type type, SharedPtr<String> literal, SharedPtr<String> file, size_t line, size_t column)
        : m_type { type }
        , m_literal { literal }
        , m_file { file }
        , m_line { line }
        , m_column { column } {
        assert(literal);
        assert(file);
    }

    Token(Type type, char literal, SharedPtr<String> file, size_t line, size_t column)
        : m_type { type }
        , m_literal { new String(literal) }
        , m_file { file }
        , m_line { line }
        , m_column { column } {
        assert(file);
    }

    Token(Type type, long long fixnum, SharedPtr<String> file, size_t line, size_t column)
        : m_type { type }
        , m_fixnum { fixnum }
        , m_file { file }
        , m_line { line }
        , m_column { column } {
        assert(file);
    }

    Token(Type type, double dbl, SharedPtr<String> file, size_t line, size_t column)
        : m_type { type }
        , m_double { dbl }
        , m_file { file }
        , m_line { line }
        , m_column { column } {
        assert(file);
    }

    static Token invalid() { return Token {}; }

    operator bool() const { return is_valid(); }

    Type type() const { return m_type; }
    void set_type(Token::Type type) { m_type = type; }

    const char *literal() const {
        if (!m_literal)
            return nullptr;
        return m_literal.value()->c_str();
    }

    const char *literal_or_blank() const {
        if (!m_literal)
            return "";
        return m_literal.value()->c_str();
    }

    SharedPtr<String> literal_string() const {
        assert(m_literal);
        return m_literal.value();
    }

    bool has_literal() {
        return !!m_literal;
    }

    const char *type_value() const {
        switch (m_type) {
        case Type::AliasKeyword:
            return "alias";
        case Type::And:
            return "&&";
        case Type::AndEqual:
            return "&&=";
        case Type::AndKeyword:
            return "and";
        case Type::Arrow:
            return "->";
        case Type::BackRef:
            return "back_ref";
        case Type::BareName:
            return "name";
        case Type::BeginKeyword:
            return "begin";
        case Type::BEGINKeyword:
            return "BEGIN";
        case Type::Bignum:
            return "bignum";
        case Type::BitwiseAnd:
            return "&";
        case Type::BitwiseAndEqual:
            return "&=";
        case Type::BitwiseOr:
            return "|";
        case Type::BitwiseOrEqual:
            return "|=";
        case Type::BitwiseXor:
            return "^";
        case Type::BitwiseXorEqual:
            return "^=";
        case Type::BreakKeyword:
            return "break";
        case Type::CaseKeyword:
            return "case";
        case Type::ClassKeyword:
            return "class";
        case Type::ClassVariable:
            return "cvar";
        case Type::Comma:
            return ",";
        case Type::Comment:
            return "comment";
        case Type::Comparison:
            return "<=>";
        case Type::ConstantResolution:
            return "::";
        case Type::Constant:
            return "constant";
        case Type::DefinedKeyword:
            return "defined?";
        case Type::DefKeyword:
            return "def";
        case Type::DivideEqual:
            return "/=";
        case Type::Divide:
            return "/";
        case Type::Doc:
            return "doc";
        case Type::DoKeyword:
            return "do";
        case Type::DotDotDot:
            return "...";
        case Type::DotDot:
            return "..";
        case Type::Dot:
            return ".";
        case Type::ElseKeyword:
            return "else";
        case Type::ElsifKeyword:
            return "elsif";
        case Type::ENCODINGKeyword:
            return "__ENCODING__";
        case Type::EndKeyword:
            return "end";
        case Type::ENDKeyword:
            return "END";
        case Type::EnsureKeyword:
            return "ensure";
        case Type::Eof:
            return "EOF";
        case Type::Eol:
            return "\n";
        case Type::EqualEqualEqual:
            return "===";
        case Type::EqualEqual:
            return "==";
        case Type::Equal:
            return "=";
        case Type::EvaluateToStringBegin:
            return "evstr";
        case Type::EvaluateToStringEnd:
            return "evstrend";
        case Type::Exponent:
            return "**";
        case Type::ExponentEqual:
            return "**=";
        case Type::FalseKeyword:
            return "false";
        case Type::FILEKeyword:
            return "__FILE__";
        case Type::Float:
            return "float";
        case Type::ForKeyword:
            return "for";
        case Type::GlobalVariable:
            return "gvar";
        case Type::GreaterThanOrEqual:
            return ">=";
        case Type::GreaterThan:
            return ">";
        case Type::HashRocket:
            return "=>";
        case Type::IfKeyword:
            return "if";
        case Type::InKeyword:
            return "in";
        case Type::InstanceVariable:
            return "ivar";
        case Type::Fixnum:
            return "fixnum";
        case Type::InterpolatedRegexpBegin:
            return "dregx";
        case Type::InterpolatedRegexpEnd:
            return "dregxend";
        case Type::InterpolatedShellBegin:
            return "dxstr";
        case Type::InterpolatedShellEnd:
            return "dxstrend";
        case Type::InterpolatedHeredocBegin:
        case Type::InterpolatedStringBegin:
            return "dstr";
        case Type::InterpolatedHeredocEnd:
        case Type::InterpolatedStringEnd:
            return "dstrend";
        case Type::InterpolatedSymbolBegin:
            return "dsym";
        case Type::InterpolatedSymbolEnd:
            return "dsymend";
        case Type::Invalid:
        case Type::InvalidCharacterEscape:
        case Type::InvalidUnicodeEscape:
            return nullptr;
        case Type::LCurlyBrace:
            return "{";
        case Type::LBracket:
            return "[";
        case Type::LBracketRBracket:
            return "[]";
        case Type::LBracketRBracketEqual:
            return "[]=";
        case Type::LeftShift:
            return "<<";
        case Type::LeftShiftEqual:
            return "<<=";
        case Type::LessThanOrEqual:
            return "<=";
        case Type::LessThan:
            return "<";
        case Type::LINEKeyword:
            return "__LINE__";
        case Type::LParen:
            return "(";
        case Type::Match:
            return "=~";
        case Type::MinusEqual:
            return "-=";
        case Type::Minus:
            return "-";
        case Type::ModuleKeyword:
            return "module";
        case Type::ModulusEqual:
            return "%=";
        case Type::Modulus:
            return "%";
        case Type::MultiplyEqual:
            return "*=";
        case Type::Multiply:
            return "*";
        case Type::NextKeyword:
            return "next";
        case Type::NilKeyword:
            return "nil";
        case Type::NotEqual:
            return "!=";
        case Type::NotKeyword:
            return "not";
        case Type::NotMatch:
            return "!~";
        case Type::Not:
            return "!";
        case Type::NthRef:
            return "nth_ref";
        case Type::Or:
            return "||";
        case Type::OrEqual:
            return "||=";
        case Type::OrKeyword:
            return "or";
        case Type::PercentLowerI:
            return "%i[";
        case Type::PercentLowerW:
            return "%w[";
        case Type::PercentUpperI:
            return "%I[";
        case Type::PercentUpperW:
            return "%W[";
        case Type::PlusEqual:
            return "+=";
        case Type::Plus:
            return "+";
        case Type::RCurlyBrace:
            return "}";
        case Type::RBracket:
            return "]";
        case Type::RedoKeyword:
            return "redo";
        case Type::RescueKeyword:
            return "rescue";
        case Type::RetryKeyword:
            return "retry";
        case Type::ReturnKeyword:
            return "return";
        case Type::RightShift:
            return ">>";
        case Type::RightShiftEqual:
            return ">>=";
        case Type::RParen:
            return ")";
        case Type::SafeNavigation:
            return "&.";
        case Type::SelfKeyword:
            return "self";
        case Type::Semicolon:
            return ";";
        case Type::String:
            return "string";
        case Type::SuperKeyword:
            return "super";
        case Type::SymbolKey:
            return "symbol_key";
        case Type::Symbol:
            return "symbol";
        case Type::TernaryColon:
            return ":";
        case Type::TernaryQuestion:
            return "?";
        case Type::ThenKeyword:
            return "then";
        case Type::Tilde:
            return "~";
        case Type::TrueKeyword:
            return "true";
        case Type::UndefKeyword:
            return "undef";
        case Type::UnlessKeyword:
            return "unless";
        case Type::UnterminatedRegexp:
            return nullptr;
        case Type::UnterminatedString:
            return nullptr;
        case Type::UntilKeyword:
            return "until";
        case Type::WhenKeyword:
            return "when";
        case Type::WhileKeyword:
            return "while";
        case Type::YieldKeyword:
            return "yield";
        }
        TM_UNREACHABLE();
    }

    bool is_assignable() const {
        switch (m_type) {
        case Type::BareName:
        case Type::ClassVariable:
        case Type::Constant:
        case Type::ConstantResolution:
        case Type::GlobalVariable:
        case Type::InstanceVariable:
            return true;
        default:
            return false;
        }
    }

    bool is_keyword() const {
        switch (m_type) {
        case Type::AliasKeyword:
        case Type::AndKeyword:
        case Type::BeginKeyword:
        case Type::BEGINKeyword:
        case Type::BreakKeyword:
        case Type::CaseKeyword:
        case Type::ClassKeyword:
        case Type::DefinedKeyword:
        case Type::DefKeyword:
        case Type::DoKeyword:
        case Type::ElseKeyword:
        case Type::ElsifKeyword:
        case Type::ENCODINGKeyword:
        case Type::EndKeyword:
        case Type::ENDKeyword:
        case Type::EnsureKeyword:
        case Type::FalseKeyword:
        case Type::FILEKeyword:
        case Type::ForKeyword:
        case Type::IfKeyword:
        case Type::InKeyword:
        case Type::LINEKeyword:
        case Type::ModuleKeyword:
        case Type::NextKeyword:
        case Type::NilKeyword:
        case Type::NotKeyword:
        case Type::OrKeyword:
        case Type::RedoKeyword:
        case Type::RescueKeyword:
        case Type::RetryKeyword:
        case Type::ReturnKeyword:
        case Type::SelfKeyword:
        case Type::SuperKeyword:
        case Type::ThenKeyword:
        case Type::TrueKeyword:
        case Type::UndefKeyword:
        case Type::UnlessKeyword:
        case Type::UntilKeyword:
        case Type::WhenKeyword:
        case Type::WhileKeyword:
        case Type::YieldKeyword:
            return true;
        default:
            return false;
        }
    }

    bool is_operator() const {
        switch (m_type) {
        case Token::Type::BitwiseAnd:
        case Token::Type::BitwiseOr:
        case Token::Type::BitwiseXor:
        case Token::Type::Comparison:
        case Token::Type::Divide:
        case Token::Type::EqualEqual:
        case Token::Type::EqualEqualEqual:
        case Token::Type::Exponent:
        case Token::Type::GreaterThan:
        case Token::Type::GreaterThanOrEqual:
        case Token::Type::LBracketRBracket:
        case Token::Type::LBracketRBracketEqual:
        case Token::Type::LeftShift:
        case Token::Type::LessThan:
        case Token::Type::LessThanOrEqual:
        case Token::Type::Match:
        case Token::Type::Minus:
        case Token::Type::Modulus:
        case Token::Type::Multiply:
        case Token::Type::NotEqual:
        case Token::Type::NotMatch:
        case Token::Type::Plus:
        case Token::Type::RightShift:
        case Token::Type::Tilde:
            return true;
        default:
            return false;
        }
    }

    bool is_bare_name() const { return m_type == Type::BareName; }
    bool is_block_arg_delimiter() const { return m_type == Type::BitwiseOr; }
    bool is_closing_token() const { return m_type == Type::RBracket || m_type == Type::RCurlyBrace || m_type == Type::RParen; }
    bool is_comma() const { return m_type == Type::Comma; }
    bool is_comment() const { return m_type == Type::Comment; }
    bool is_def_keyword() const { return m_type == Type::DefKeyword; }
    bool is_doc() const { return m_type == Type::Doc; }
    bool is_dot() const { return m_type == Type::Dot; }
    bool is_else_keyword() const { return m_type == Type::ElseKeyword; }
    bool is_elsif_keyword() const { return m_type == Type::ElsifKeyword; }
    bool is_end_keyword() const { return m_type == Type::EndKeyword; }
    bool is_end_of_expression() const { return m_type == Type::EndKeyword || m_type == Type::Eol || m_type == Type::Eof || is_expression_modifier(); }
    bool is_eof() const { return m_type == Type::Eof; }
    bool is_eol() const { return m_type == Type::Eol; }
    bool is_expression_modifier() const { return m_type == Type::IfKeyword || m_type == Type::UnlessKeyword || m_type == Type::WhileKeyword || m_type == Type::UntilKeyword; }
    bool is_hash_rocket() const { return m_type == Type::HashRocket; }
    bool is_lparen() const { return m_type == Type::LParen; }
    bool is_newline() const { return m_type == Type::Eol; }
    bool is_rparen() const { return m_type == Type::RParen; }
    bool is_semicolon() const { return m_type == Type::Semicolon; }
    bool is_splat() const { return m_type == Type::Multiply || m_type == Type::Exponent; }
    bool is_when_keyword() const { return m_type == Type::WhenKeyword; }

    bool is_valid() const {
        switch (m_type) {
        case Type::Invalid:
        case Type::InvalidCharacterEscape:
        case Type::InvalidUnicodeEscape:
        case Type::UnterminatedRegexp:
        case Type::UnterminatedString:
            return false;
        default:
            return true;
        }
    }

    bool can_follow_collapsible_newline() {
        switch (m_type) {
        case Token::Type::Dot:
        case Token::Type::RCurlyBrace:
        case Token::Type::RBracket:
        case Token::Type::RParen:
        case Token::Type::TernaryColon:
            return true;
        default:
            return false;
        }
    }

    bool can_precede_collapsible_newline() {
        switch (m_type) {
        case Token::Type::And:
        case Token::Type::AndKeyword:
        case Token::Type::Arrow:
        case Token::Type::BitwiseAnd:
        case Token::Type::BitwiseOr:
        case Token::Type::BitwiseXor:
        case Token::Type::CaseKeyword:
        case Token::Type::Comma:
        case Token::Type::Comparison:
        case Token::Type::ConstantResolution:
        case Token::Type::Divide:
        case Token::Type::DivideEqual:
        case Token::Type::Dot:
        case Token::Type::DotDot:
        case Token::Type::Equal:
        case Token::Type::EqualEqual:
        case Token::Type::EqualEqualEqual:
        case Token::Type::Exponent:
        case Token::Type::ExponentEqual:
        case Token::Type::GreaterThan:
        case Token::Type::GreaterThanOrEqual:
        case Token::Type::HashRocket:
        case Token::Type::InKeyword:
        case Token::Type::LCurlyBrace:
        case Token::Type::LBracket:
        case Token::Type::LeftShift:
        case Token::Type::LessThan:
        case Token::Type::LessThanOrEqual:
        case Token::Type::LParen:
        case Token::Type::Match:
        case Token::Type::Minus:
        case Token::Type::MinusEqual:
        case Token::Type::Modulus:
        case Token::Type::ModulusEqual:
        case Token::Type::Multiply:
        case Token::Type::MultiplyEqual:
        case Token::Type::Not:
        case Token::Type::NotEqual:
        case Token::Type::NotMatch:
        case Token::Type::Or:
        case Token::Type::OrKeyword:
        case Token::Type::Plus:
        case Token::Type::PlusEqual:
        case Token::Type::RightShift:
        case Token::Type::SafeNavigation:
        case Token::Type::TernaryColon:
        case Token::Type::TernaryQuestion:
        case Token::Type::Tilde:
            return true;
        default:
            return false;
        }
    }

    bool can_have_doc() {
        switch (m_type) {
        case Token::Type::ClassKeyword:
        case Token::Type::DefKeyword:
        case Token::Type::ModuleKeyword:
            return true;
        default:
            return false;
        }
    }

    bool can_be_first_arg_of_implicit_call() {
        switch (m_type) {
        case Token::Type::Arrow:
        case Token::Type::BareName:
        case Token::Type::Bignum:
        case Token::Type::ClassVariable:
        case Token::Type::Constant:
        case Token::Type::ConstantResolution:
        case Token::Type::DefKeyword:
        case Token::Type::DefinedKeyword:
        case Token::Type::DoKeyword:
        case Token::Type::ENCODINGKeyword:
        case Token::Type::FalseKeyword:
        case Token::Type::FILEKeyword:
        case Token::Type::Fixnum:
        case Token::Type::Float:
        case Token::Type::GlobalVariable:
        case Token::Type::InstanceVariable:
        case Token::Type::InterpolatedRegexpBegin:
        case Token::Type::InterpolatedShellBegin:
        case Token::Type::InterpolatedStringBegin:
        case Token::Type::InterpolatedSymbolBegin:
        case Token::Type::LCurlyBrace:
        case Token::Type::LBracket:
        case Token::Type::LBracketRBracket:
        case Token::Type::LINEKeyword:
        case Token::Type::LParen:
        case Token::Type::Multiply:
        case Token::Type::NilKeyword:
        case Token::Type::Not:
        case Token::Type::NotKeyword:
        case Token::Type::PercentLowerI:
        case Token::Type::PercentLowerW:
        case Token::Type::PercentUpperI:
        case Token::Type::PercentUpperW:
        case Token::Type::String:
        case Token::Type::SuperKeyword:
        case Token::Type::Symbol:
        case Token::Type::SymbolKey:
        case Token::Type::Tilde:
        case Token::Type::TrueKeyword:
            return true;
        default:
            return false;
        }
    }

    void set_literal(const char *literal) { m_literal = new String(literal); }
    void set_literal(SharedPtr<String> literal) { m_literal = literal; }
    void set_literal(String literal) { m_literal = new String(literal); }

    Optional<SharedPtr<String>> doc() const { return m_doc; }
    void set_doc(SharedPtr<String> doc) { m_doc = doc; }

    long long get_fixnum() const { return m_fixnum; }
    double get_double() const { return m_double; }

    SharedPtr<String> file() const { return m_file; }
    size_t line() const { return m_line; }
    size_t column() const { return m_column; }

    bool whitespace_precedes() const { return m_whitespace_precedes; }
    void set_whitespace_precedes(bool whitespace_precedes) { m_whitespace_precedes = whitespace_precedes; }

    void validate();

private:
    Type m_type { Type::Invalid };
    Optional<SharedPtr<String>> m_literal {};
    Optional<SharedPtr<String>> m_doc {};
    long long m_fixnum { 0 };
    double m_double { 0 };
    SharedPtr<String> m_file;
    size_t m_line { 0 };
    size_t m_column { 0 };
    bool m_whitespace_precedes { false };
};
}
