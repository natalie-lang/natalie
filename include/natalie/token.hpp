#pragma once

#include "natalie.hpp"

namespace Natalie {

struct Token : public gc {
    enum class Type {
        AliasKeyword,
        And,
        AndKeyword,
        Arrow,
        BeginKeyword,
        BEGINKeyword,
        BinaryAnd,
        BinaryOnesComplement,
        BinaryOr,
        BinaryXor,
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
        Identifier,
        IfKeyword,
        InKeyword,
        InstanceVariable,
        Integer,
        Invalid,
        LBrace,
        LBracket,
        LeftShift,
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
        OrKeyword,
        PercentLowerI,
        PercentLowerW,
        PercentUpperI,
        PercentUpperW,
        Plus,
        PlusEqual,
        RBrace,
        RBracket,
        RedoKeyword,
        Regexp,
        RescueKeyword,
        RetryKeyword,
        ReturnKeyword,
        RightShift,
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

    Token(Type type, size_t line, size_t column)
        : m_type { type }
        , m_line { line }
        , m_column { column } { }

    Token(Type type, const char *literal, size_t line, size_t column)
        : m_type { type }
        , m_literal { literal }
        , m_line { line }
        , m_column { column } {
        assert(m_literal);
    }

    Token(Type type, char literal, size_t line, size_t column)
        : m_type { type }
        , m_line { line }
        , m_column { column } {
        char buf[2] = { literal, 0 };
        m_literal = GC_STRDUP(buf);
    }

    Token(Type type, nat_int_t integer, size_t line, size_t column)
        : m_type { type }
        , m_integer { integer }
        , m_line { line }
        , m_column { column } { }

    Token(Type type, double dbl, size_t line, size_t column)
        : m_type { type }
        , m_double { dbl }
        , m_line { line }
        , m_column { column } { }

    static Token invalid() { return Token { Token::Type::Invalid, 0, 0 }; }

    Type type() { return m_type; }
    void set_type(Token::Type type) { m_type = type; }

    const char *literal() { return m_literal; }
    void set_literal(const char *literal) { m_literal = literal; }
    void set_literal(std::string literal) { m_literal = GC_STRDUP(literal.c_str()); }

    nat_int_t get_integer() { return m_integer; }
    double get_double() { return m_double; }

    SymbolValue *type_value(Env *env) {
        switch (m_type) {
        case Type::AliasKeyword:
            return SymbolValue::intern(env, "alias");
        case Type::AndKeyword:
            return SymbolValue::intern(env, "and");
        case Type::And:
            return SymbolValue::intern(env, "&&");
        case Type::Arrow:
            return SymbolValue::intern(env, "->");
        case Type::BeginKeyword:
            return SymbolValue::intern(env, "begin");
        case Type::BEGINKeyword:
            return SymbolValue::intern(env, "BEGIN");
        case Type::BinaryAnd:
            return SymbolValue::intern(env, "&");
        case Type::BinaryOnesComplement:
            return SymbolValue::intern(env, "~");
        case Type::BinaryOr:
            return SymbolValue::intern(env, "|");
        case Type::BinaryXor:
            return SymbolValue::intern(env, "^");
        case Type::BreakKeyword:
            return SymbolValue::intern(env, "break");
        case Type::CaseKeyword:
            return SymbolValue::intern(env, "case");
        case Type::ClassKeyword:
            return SymbolValue::intern(env, "class");
        case Type::ClassVariable:
            return SymbolValue::intern(env, "cvar");
        case Type::Comma:
            return SymbolValue::intern(env, ",");
        case Type::Comment:
            return SymbolValue::intern(env, "comment");
        case Type::Comparison:
            return SymbolValue::intern(env, "<=>");
        case Type::ConstantResolution:
            return SymbolValue::intern(env, "::");
        case Type::Constant:
            return SymbolValue::intern(env, "constant");
        case Type::DefinedKeyword:
            return SymbolValue::intern(env, "defined?");
        case Type::DefKeyword:
            return SymbolValue::intern(env, "def");
        case Type::DivideEqual:
            return SymbolValue::intern(env, "/=");
        case Type::Divide:
            return SymbolValue::intern(env, "/");
        case Type::DoKeyword:
            return SymbolValue::intern(env, "do");
        case Type::DotDotDot:
            return SymbolValue::intern(env, "...");
        case Type::DotDot:
            return SymbolValue::intern(env, "..");
        case Type::Dot:
            return SymbolValue::intern(env, ".");
        case Type::ElseKeyword:
            return SymbolValue::intern(env, "else");
        case Type::ElsifKeyword:
            return SymbolValue::intern(env, "elsif");
        case Type::ENCODINGKeyword:
            return SymbolValue::intern(env, "__ENCODING__");
        case Type::EndKeyword:
            return SymbolValue::intern(env, "end");
        case Type::ENDKeyword:
            return SymbolValue::intern(env, "END");
        case Type::EnsureKeyword:
            return SymbolValue::intern(env, "ensure");
        case Type::Eof:
            return SymbolValue::intern(env, "EOF");
        case Type::Eol:
            return SymbolValue::intern(env, "\n");
        case Type::EqualEqualEqual:
            return SymbolValue::intern(env, "===");
        case Type::EqualEqual:
            return SymbolValue::intern(env, "==");
        case Type::Equal:
            return SymbolValue::intern(env, "=");
        case Type::ExponentEqual:
            return SymbolValue::intern(env, "**=");
        case Type::Exponent:
            return SymbolValue::intern(env, "**");
        case Type::FalseKeyword:
            return SymbolValue::intern(env, "false");
        case Type::FILEKeyword:
            return SymbolValue::intern(env, "__FILE__");
        case Type::Float:
            return SymbolValue::intern(env, "float");
        case Type::ForKeyword:
            return SymbolValue::intern(env, "for");
        case Type::GlobalVariable:
            return SymbolValue::intern(env, "gvar");
        case Type::GreaterThanOrEqual:
            return SymbolValue::intern(env, ">=");
        case Type::GreaterThan:
            return SymbolValue::intern(env, ">");
        case Type::HashRocket:
            return SymbolValue::intern(env, "=>");
        case Type::Identifier:
            return SymbolValue::intern(env, "identifier");
        case Type::IfKeyword:
            return SymbolValue::intern(env, "if");
        case Type::InKeyword:
            return SymbolValue::intern(env, "in");
        case Type::InstanceVariable:
            return SymbolValue::intern(env, "ivar");
        case Type::Integer:
            return SymbolValue::intern(env, "integer");
        case Type::Invalid:
            env->raise("SyntaxError", "%d: syntax error, unexpected '%s'", m_line + 1, m_literal);
        case Type::LBrace:
            return SymbolValue::intern(env, "{");
        case Type::LBracket:
            return SymbolValue::intern(env, "[");
        case Type::LeftShift:
            return SymbolValue::intern(env, "<<");
        case Type::LessThanOrEqual:
            return SymbolValue::intern(env, "<=");
        case Type::LessThan:
            return SymbolValue::intern(env, "<");
        case Type::LINEKeyword:
            return SymbolValue::intern(env, "__LINE__");
        case Type::LParen:
            return SymbolValue::intern(env, "(");
        case Type::Match:
            return SymbolValue::intern(env, "=~");
        case Type::MinusEqual:
            return SymbolValue::intern(env, "-=");
        case Type::Minus:
            return SymbolValue::intern(env, "-");
        case Type::ModuleKeyword:
            return SymbolValue::intern(env, "module");
        case Type::ModulusEqual:
            return SymbolValue::intern(env, "%=");
        case Type::Modulus:
            return SymbolValue::intern(env, "%");
        case Type::MultiplyEqual:
            return SymbolValue::intern(env, "*=");
        case Type::Multiply:
            return SymbolValue::intern(env, "*");
        case Type::NextKeyword:
            return SymbolValue::intern(env, "next");
        case Type::NilKeyword:
            return SymbolValue::intern(env, "nil");
        case Type::NotEqual:
            return SymbolValue::intern(env, "!=");
        case Type::NotKeyword:
            return SymbolValue::intern(env, "not");
        case Type::NotMatch:
            return SymbolValue::intern(env, "!~");
        case Type::Not:
            return SymbolValue::intern(env, "!");
        case Type::OrKeyword:
            return SymbolValue::intern(env, "or");
        case Type::Or:
            return SymbolValue::intern(env, "||");
        case Type::PercentLowerI:
            return SymbolValue::intern(env, "%i");
        case Type::PercentLowerW:
            return SymbolValue::intern(env, "%w");
        case Type::PercentUpperI:
            return SymbolValue::intern(env, "%I");
        case Type::PercentUpperW:
            return SymbolValue::intern(env, "%W");
        case Type::PlusEqual:
            return SymbolValue::intern(env, "+=");
        case Type::Plus:
            return SymbolValue::intern(env, "+");
        case Type::RBrace:
            return SymbolValue::intern(env, "}");
        case Type::RBracket:
            return SymbolValue::intern(env, "]");
        case Type::RedoKeyword:
            return SymbolValue::intern(env, "redo");
        case Type::Regexp:
            return SymbolValue::intern(env, "regexp");
        case Type::RescueKeyword:
            return SymbolValue::intern(env, "rescue");
        case Type::RetryKeyword:
            return SymbolValue::intern(env, "retry");
        case Type::ReturnKeyword:
            return SymbolValue::intern(env, "return");
        case Type::RightShift:
            return SymbolValue::intern(env, ">>");
        case Type::RParen:
            return SymbolValue::intern(env, ")");
        case Type::SafeNavigation:
            return SymbolValue::intern(env, "&.");
        case Type::SelfKeyword:
            return SymbolValue::intern(env, "self");
        case Type::Semicolon:
            return SymbolValue::intern(env, ";");
        case Type::String:
            return SymbolValue::intern(env, "string");
        case Type::SuperKeyword:
            return SymbolValue::intern(env, "super");
        case Type::SymbolKey:
            return SymbolValue::intern(env, "symbol_key");
        case Type::Symbol:
            return SymbolValue::intern(env, "symbol");
        case Type::TernaryColon:
            return SymbolValue::intern(env, ":");
        case Type::TernaryQuestion:
            return SymbolValue::intern(env, "?");
        case Type::ThenKeyword:
            return SymbolValue::intern(env, "then");
        case Type::TrueKeyword:
            return SymbolValue::intern(env, "true");
        case Type::UndefKeyword:
            return SymbolValue::intern(env, "undef");
        case Type::UnlessKeyword:
            return SymbolValue::intern(env, "unless");
        case Type::UnterminatedRegexp:
            env->raise("SyntaxError", "unterminated regexp meets end of file");
        case Type::UnterminatedString:
            env->raise("SyntaxError", "unterminated string meets end of file at line %i and column %i: %s", m_line, m_column, m_literal);
        case Type::UntilKeyword:
            return SymbolValue::intern(env, "until");
        case Type::WhenKeyword:
            return SymbolValue::intern(env, "when");
        case Type::WhileKeyword:
            return SymbolValue::intern(env, "while");
        case Type::YieldKeyword:
            return SymbolValue::intern(env, "yield");
        }
        NAT_UNREACHABLE();
    }

    Value *to_ruby(Env *env, bool with_line_and_column_numbers = false) {
        if (m_type == Type::Eof)
            return env->nil_obj();
        Value *type = type_value(env);
        auto hash = new HashValue { env };
        hash->put(env, SymbolValue::intern(env, "type"), type);
        switch (m_type) {
        case Type::PercentLowerI:
        case Type::PercentUpperI:
        case Type::PercentLowerW:
        case Type::PercentUpperW:
        case Type::Regexp:
        case Type::String:
            hash->put(env, SymbolValue::intern(env, "literal"), new StringValue { env, m_literal });
            break;
        case Type::ClassVariable:
        case Type::Constant:
        case Type::GlobalVariable:
        case Type::Identifier:
        case Type::InstanceVariable:
        case Type::Symbol:
        case Type::SymbolKey:
            hash->put(env, SymbolValue::intern(env, "literal"), SymbolValue::intern(env, m_literal));
            break;
        case Type::Float:
            hash->put(env, SymbolValue::intern(env, "literal"), new FloatValue { env, m_double });
            break;
        case Type::Integer:
            hash->put(env, SymbolValue::intern(env, "literal"), new IntegerValue { env, m_integer });
            break;
        default:
            void();
        }
        if (with_line_and_column_numbers) {
            hash->put(env, SymbolValue::intern(env, "line"), new IntegerValue { env, static_cast<nat_int_t>(m_line) });
            hash->put(env, SymbolValue::intern(env, "column"), new IntegerValue { env, static_cast<nat_int_t>(m_column) });
        }
        return hash;
    }

    bool is_comma() { return m_type == Type::Comma; }
    bool is_comment() { return m_type == Type::Comment; }
    bool is_else_keyword() { return m_type == Type::ElseKeyword; }
    bool is_elsif_keyword() { return m_type == Type::ElsifKeyword; }
    bool is_end_keyword() { return m_type == Type::EndKeyword; }
    bool is_eof() { return m_type == Type::Eof; }
    bool is_eol() { return m_type == Type::Eol; }
    bool is_end_of_expression() { return m_type == Type::Eol || m_type == Type::Eof; }
    bool is_identifier() { return m_type == Type::Identifier; }
    bool is_lparen() { return m_type == Type::LParen; }
    bool is_newline() { return m_type == Type::Eol; }
    bool is_rparen() { return m_type == Type::RParen; }
    bool is_semicolon() { return m_type == Type::Semicolon; }
    bool is_valid() { return m_type != Type::Invalid; }

    void validate(Env *env) {
        if (!is_valid()) {
            type_value(env);
            NAT_UNREACHABLE();
        }
    }

    bool can_precede_method_name() {
        return m_type == Token::Type::Dot || m_type == Token::Type::ConstantResolution || m_type == Token::Type::DefKeyword;
    }

    bool can_follow_collapsible_newline() {
        return m_type == Token::Type::RBrace
            || m_type == Token::Type::RBracket
            || m_type == Token::Type::RParen
            || m_type == Token::Type::TernaryColon;
    }

    bool can_precede_collapsible_newline() {
        return m_type == Token::Type::And
            || m_type == Token::Type::AndKeyword
            || m_type == Token::Type::Arrow
            || m_type == Token::Type::BinaryAnd
            || m_type == Token::Type::BinaryOnesComplement
            || m_type == Token::Type::BinaryOr
            || m_type == Token::Type::BinaryXor
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
            || m_type == Token::Type::LBrace
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

    bool has_sign() { return m_has_sign; }
    void set_has_sign(bool has_sign) { m_has_sign = has_sign; }

    size_t line() { return m_line; }
    size_t column() { return m_column; }

private:
    Type m_type { Type::Invalid };
    const char *m_literal { nullptr };
    nat_int_t m_integer { 0 };
    double m_double { 0 };
    bool m_has_sign { false };
    size_t m_line { 0 };
    size_t m_column { 0 };
};
}
