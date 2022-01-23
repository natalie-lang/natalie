#pragma once

#include "natalie/float_object.hpp"
#include "natalie/gc.hpp"
#include "natalie/hash_object.hpp"
#include "natalie/integer_object.hpp"
#include "natalie/object.hpp"
#include "natalie/regexp_object.hpp"
#include "natalie/sexp_object.hpp"
#include "natalie/string_object.hpp"
#include "natalie/symbol_object.hpp"
#include "tm/optional.hpp"

namespace Natalie {

class Token : public Cell {
public:
    NAT_MAKE_NONCOPYABLE(Token);

    enum class Type {
        AliasKeyword,
        And,
        AndEqual,
        AndKeyword,
        Arrow,
        BareName,
        BeginKeyword,
        BEGINKeyword,
        BitwiseAnd,
        BitwiseAndEqual,
        BinaryOnesComplement,
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
        DoKeyword,
        Dot,
        DotDot,
        DotDotDot,
        DoubleQuotedString,
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
        Float,
        ForKeyword,
        GlobalVariable,
        GreaterThan,
        GreaterThanOrEqual,
        HashRocket,
        IfKeyword,
        InKeyword,
        InstanceVariable,
        Integer,
        InterpolatedRegexpBegin,
        InterpolatedRegexpEnd,
        InterpolatedShellBegin,
        InterpolatedShellEnd,
        InterpolatedStringBegin,
        InterpolatedStringEnd,
        Invalid,
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
        Regexp,
        RescueKeyword,
        RetryKeyword,
        ReturnKeyword,
        RightShift,
        RightShiftEqual,
        RParen,
        SafeNavigation,
        SelfKeyword,
        Semicolon,
        Shell,
        String,
        SuperKeyword,
        Symbol,
        SymbolKey,
        TernaryColon,
        TernaryQuestion,
        ThenKeyword,
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

    Token(Type type, SharedPtr<String> file, size_t line, size_t column)
        : m_type { type }
        , m_file { file }
        , m_line { line }
        , m_column { column } {
        assert(file);
    }

    Token(Type type, const char *literal, SharedPtr<String> file, size_t line, size_t column)
        : m_type { type }
        , m_literal { new ManagedString(literal) }
        , m_file { file }
        , m_line { line }
        , m_column { column } {
        assert(literal);
        assert(file);
    }

    Token(Type type, const ManagedString *literal, SharedPtr<String> file, size_t line, size_t column)
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
        , m_literal { new ManagedString(literal) }
        , m_file { file }
        , m_line { line }
        , m_column { column } {
        assert(file);
    }

    Token(Type type, nat_int_t integer, SharedPtr<String> file, size_t line, size_t column)
        : m_type { type }
        , m_integer { integer }
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

    static Token *invalid() { return new Token { Token::Type::Invalid, nullptr, 0, 0 }; }

    Type type() { return m_type; }
    void set_type(Token::Type type) { m_type = type; }

    const char *literal() {
        if (!m_literal)
            return nullptr;
        return m_literal.value()->c_str();
    }

    const char *literal_or_blank() {
        if (!m_literal)
            return "";
        return m_literal.value()->c_str();
    }

    const ManagedString *literal_string() {
        assert(m_literal);
        return m_literal.value();
    }

    void set_literal(const char *literal) { m_literal = new ManagedString(literal); }
    void set_literal(const ManagedString *literal) { m_literal = literal; }

    const ManagedString *options() { return m_options ? m_options.value() : nullptr; }
    void set_options(const ManagedString *options) { m_options = options; }

    nat_int_t get_integer() const { return m_integer; }
    double get_double() const { return m_double; }

    const char *type_value() {
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
        case Type::BareName:
            return "name";
        case Type::BeginKeyword:
            return "begin";
        case Type::BEGINKeyword:
            return "BEGIN";
        case Type::BitwiseAnd:
            return "&";
        case Type::BitwiseAndEqual:
            return "&=";
        case Type::BinaryOnesComplement:
            return "~";
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
        case Type::DoKeyword:
            return "do";
        case Type::DotDotDot:
            return "...";
        case Type::DotDot:
            return "..";
        case Type::Dot:
            return ".";
        case Type::DoubleQuotedString:
            NAT_UNREACHABLE(); // converted to InterpolatedStringBegin/InterpolatedStringEnd
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
        case Type::Integer:
            return "integer";
        case Type::InterpolatedRegexpBegin:
            return "dregx";
        case Type::InterpolatedRegexpEnd:
            return "dregxend";
        case Type::InterpolatedShellBegin:
            return "dxstr";
        case Type::InterpolatedShellEnd:
            return "dxstrend";
        case Type::InterpolatedStringBegin:
            return "dstr";
        case Type::InterpolatedStringEnd:
            return "dstrend";
        case Type::Invalid:
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
        case Type::Or:
            return "||";
        case Type::OrEqual:
            return "||=";
        case Type::OrKeyword:
            return "or";
        case Type::PercentLowerI:
            return "%i";
        case Type::PercentLowerW:
            return "%w";
        case Type::PercentUpperI:
            return "%I";
        case Type::PercentUpperW:
            return "%W";
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
        case Type::Regexp:
            NAT_UNREACHABLE(); // converted to InterpolatedRegexpBegin/InterpolatedRegexpEnd
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
        case Type::Shell:
            NAT_UNREACHABLE(); // converted to InterpolatedShellBegin/InterpolatedShellEnd
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
        NAT_UNREACHABLE();
    }

    Value to_ruby(Env *env, bool with_line_and_column_numbers = false) {
        if (m_type == Type::Eof)
            return NilObject::the();
        validate_or_raise(env);
        const char *type = type_value();
        auto hash = new HashObject {};
        hash->put(env, "type"_s, SymbolObject::intern(type));
        switch (m_type) {
        case Type::PercentLowerI:
        case Type::PercentUpperI:
        case Type::PercentLowerW:
        case Type::PercentUpperW:
        case Type::Regexp:
        case Type::String:
            hash->put(env, "literal"_s, new StringObject { literal_or_blank() });
            break;
        case Type::BareName:
        case Type::ClassVariable:
        case Type::Constant:
        case Type::GlobalVariable:
        case Type::InstanceVariable:
        case Type::Symbol:
        case Type::SymbolKey:
            hash->put(env, "literal"_s, SymbolObject::intern(literal_or_blank()));
            break;
        case Type::Float:
            hash->put(env, "literal"_s, new FloatObject { m_double });
            break;
        case Type::Integer:
            hash->put(env, "literal"_s, Value::integer(m_integer));
            break;
        case Type::InterpolatedRegexpEnd:
            if (m_options)
                hash->put(env, "options"_s, new StringObject { m_options.value() });
            break;
        default:
            void();
        }
        if (with_line_and_column_numbers) {
            hash->put(env, "line"_s, Value::integer(static_cast<nat_int_t>(m_line)));
            hash->put(env, "column"_s, Value::integer(static_cast<nat_int_t>(m_column)));
        }
        return hash;
    }

    bool is_assignable() const {
        switch (m_type) {
        case Type::BareName:
        case Type::ClassVariable:
        case Type::Constant:
        case Type::GlobalVariable:
        case Type::InstanceVariable:
            return true;
        default:
            return false;
        }
    }

    bool is_operator() const {
        switch (m_type) {
        case Token::Type::BinaryOnesComplement:
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
            return true;
        default:
            return false;
        }
    }

    bool is_bare_name() { return m_type == Type::BareName; }
    bool is_closing_token() { return m_type == Type::RBracket || m_type == Type::RCurlyBrace || m_type == Type::RParen; }
    bool is_comma() { return m_type == Type::Comma; }
    bool is_comment() { return m_type == Type::Comment; }
    bool is_else_keyword() { return m_type == Type::ElseKeyword; }
    bool is_elsif_keyword() { return m_type == Type::ElsifKeyword; }
    bool is_end_keyword() { return m_type == Type::EndKeyword; }
    bool is_end_of_expression() { return m_type == Type::EndKeyword || m_type == Type::Eol || m_type == Type::Eof || is_expression_modifier(); }
    bool is_eof() { return m_type == Type::Eof; }
    bool is_eol() { return m_type == Type::Eol; }
    bool is_expression_modifier() { return m_type == Type::IfKeyword || m_type == Type::UnlessKeyword || m_type == Type::WhileKeyword || m_type == Type::UntilKeyword; }
    bool is_lparen() { return m_type == Type::LParen; }
    bool is_newline() { return m_type == Type::Eol; }
    bool is_rparen() { return m_type == Type::RParen; }
    bool is_semicolon() { return m_type == Type::Semicolon; }
    bool is_splat() { return m_type == Type::Multiply || m_type == Type::Exponent; }
    bool is_valid() { return m_type != Type::Invalid; }
    bool is_when_keyword() { return m_type == Type::WhenKeyword; }

    void validate();
    void validate_or_raise(Env *env);

    bool can_follow_collapsible_newline() {
        return m_type == Token::Type::Dot
            || m_type == Token::Type::RCurlyBrace
            || m_type == Token::Type::RBracket
            || m_type == Token::Type::RParen
            || m_type == Token::Type::TernaryColon;
    }

    bool can_precede_collapsible_newline() {
        return m_type == Token::Type::And
            || m_type == Token::Type::AndKeyword
            || m_type == Token::Type::Arrow
            || m_type == Token::Type::BitwiseAnd
            || m_type == Token::Type::BinaryOnesComplement
            || m_type == Token::Type::BitwiseOr
            || m_type == Token::Type::BitwiseXor
            || m_type == Token::Type::Comma
            || m_type == Token::Type::Comparison
            || m_type == Token::Type::ConstantResolution
            || m_type == Token::Type::Divide
            || m_type == Token::Type::DivideEqual
            || m_type == Token::Type::Dot
            || m_type == Token::Type::DotDot
            || m_type == Token::Type::Equal
            || m_type == Token::Type::EqualEqual
            || m_type == Token::Type::EqualEqualEqual
            || m_type == Token::Type::Exponent
            || m_type == Token::Type::ExponentEqual
            || m_type == Token::Type::GreaterThan
            || m_type == Token::Type::GreaterThanOrEqual
            || m_type == Token::Type::HashRocket
            || m_type == Token::Type::InKeyword
            || m_type == Token::Type::LCurlyBrace
            || m_type == Token::Type::LBracket
            || m_type == Token::Type::LeftShift
            || m_type == Token::Type::LessThan
            || m_type == Token::Type::LessThanOrEqual
            || m_type == Token::Type::LParen
            || m_type == Token::Type::Match
            || m_type == Token::Type::Minus
            || m_type == Token::Type::MinusEqual
            || m_type == Token::Type::Modulus
            || m_type == Token::Type::ModulusEqual
            || m_type == Token::Type::Multiply
            || m_type == Token::Type::MultiplyEqual
            || m_type == Token::Type::Not
            || m_type == Token::Type::NotEqual
            || m_type == Token::Type::NotMatch
            || m_type == Token::Type::Or
            || m_type == Token::Type::OrKeyword
            || m_type == Token::Type::Plus
            || m_type == Token::Type::PlusEqual
            || m_type == Token::Type::RightShift
            || m_type == Token::Type::SafeNavigation
            || m_type == Token::Type::TernaryColon
            || m_type == Token::Type::TernaryQuestion;
    }

    bool has_sign() const { return m_has_sign; }
    void set_has_sign(bool has_sign) { m_has_sign = has_sign; }

    SharedPtr<String> file() { return m_file; }
    size_t line() const { return m_line; }
    size_t column() const { return m_column; }

    bool whitespace_precedes() const { return m_whitespace_precedes; }
    void set_whitespace_precedes(bool whitespace_precedes) { m_whitespace_precedes = whitespace_precedes; }

    bool can_have_interpolation() {
        switch (m_type) {
        case Token::Type::DoubleQuotedString:
        case Token::Type::Regexp:
        case Token::Type::Shell:
            return true;
        default:
            return false;
        }
    }

    bool can_be_first_arg_of_implicit_call() {
        switch (m_type) {
        case Token::Type::BareName:
        case Token::Type::ClassVariable:
        case Token::Type::Constant:
        case Token::Type::ConstantResolution:
        case Token::Type::DefinedKeyword:
        case Token::Type::DoubleQuotedString:
        case Token::Type::ENCODINGKeyword:
        case Token::Type::FalseKeyword:
        case Token::Type::FILEKeyword:
        case Token::Type::Float:
        case Token::Type::GlobalVariable:
        case Token::Type::InstanceVariable:
        case Token::Type::Integer:
        case Token::Type::InterpolatedRegexpBegin:
        case Token::Type::InterpolatedShellBegin:
        case Token::Type::InterpolatedStringBegin:
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
        case Token::Type::Regexp:
        case Token::Type::SelfKeyword:
        case Token::Type::Shell:
        case Token::Type::String:
        case Token::Type::SuperKeyword:
        case Token::Type::Symbol:
        case Token::Type::SymbolKey:
        case Token::Type::TrueKeyword:
            return true;
        default:
            return false;
        }
    }

    virtual void gc_inspect(char *buf, size_t len) const override final {
        snprintf(buf, len,
            "<Token %p type=%d literal='%s' m_integer=%lli m_double=%f m_has_sign=%d>",
            this,
            (int)m_type,
            m_literal ? m_literal.value()->c_str() : "",
            m_integer,
            m_double,
            m_has_sign);
    }

    virtual void visit_children(Visitor &visitor) override final {
        if (m_literal)
            visitor.visit(m_literal.value());
        if (m_options)
            visitor.visit(m_options.value());
    }

private:
    Type m_type { Type::Invalid };
    Optional<const ManagedString *> m_literal {};
    Optional<const ManagedString *> m_options {};
    nat_int_t m_integer { 0 };
    double m_double { 0 };
    bool m_has_sign { false };
    SharedPtr<String> m_file;
    size_t m_line { 0 };
    size_t m_column { 0 };
    bool m_whitespace_precedes { false };
};
}
