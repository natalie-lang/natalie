#pragma once

#include "natalie.hpp"

namespace Natalie {

struct Token : public gc {
    enum class Type {
        And,
        Arrow,
        BinaryAnd,
        BinaryOr,
        BinaryXor,
        BinaryOnesComplement,
        ClassVariable,
        Comma,
        Comment,
        Comparison,
        Constant,
        ConstantResolution,
        Divide,
        DivideEqual,
        Dot,
        DotDot,
        Eof,
        Eol,
        Equal,
        EqualEqual,
        EqualEqualEqual,
        Exponent,
        ExponentEqual,
        Float,
        GreaterThan,
        GreaterThanOrEqual,
        GlobalVariable,
        HashRocket,
        Identifier,
        InstanceVariable,
        Integer,
        Invalid,
        Keyword,
        LBrace,
        LBracket,
        LParen,
        LeftShift,
        LessThan,
        LessThanOrEqual,
        Match,
        Minus,
        MinusEqual,
        Modulus,
        ModulusEqual,
        Multiply,
        MultiplyEqual,
        Not,
        NotEqual,
        NotMatch,
        Or,
        PercentLowerI,
        PercentLowerW,
        PercentUpperI,
        PercentUpperW,
        Plus,
        PlusEqual,
        RBrace,
        RBracket,
        Regexp,
        RightShift,
        RParen,
        SafeNavigation,
        Semicolon,
        String,
        Symbol,
        SymbolKey,
        TernaryColon,
        TernaryQuestion,
        UnterminatedRegexp,
        UnterminatedString,
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

    Value *type_value(Env *env) {
        switch (m_type) {
        case Type::And:
            return SymbolValue::intern(env, "&&");
        case Type::Arrow:
            return SymbolValue::intern(env, "->");
        case Type::BinaryAnd:
            return SymbolValue::intern(env, "&");
        case Type::BinaryOr:
            return SymbolValue::intern(env, "|");
        case Type::BinaryXor:
            return SymbolValue::intern(env, "^");
        case Type::BinaryOnesComplement:
            return SymbolValue::intern(env, "~");
        case Type::ClassVariable:
            return SymbolValue::intern(env, "cvar");
        case Type::Comma:
            return SymbolValue::intern(env, ",");
        case Type::Comment:
            return env->nil_obj();
        case Type::Comparison:
            return SymbolValue::intern(env, "<=>");
        case Type::Constant:
            return SymbolValue::intern(env, "constant");
        case Type::ConstantResolution:
            return SymbolValue::intern(env, "::");
        case Type::Divide:
            return SymbolValue::intern(env, "/");
        case Type::DivideEqual:
            return SymbolValue::intern(env, "/=");
        case Type::Dot:
            return SymbolValue::intern(env, ".");
        case Type::DotDot:
            return SymbolValue::intern(env, "..");
        case Type::Eol:
            return SymbolValue::intern(env, "\n");
        case Type::Eof:
            return env->nil_obj();
        case Type::Equal:
            return SymbolValue::intern(env, "=");
        case Type::EqualEqual:
            return SymbolValue::intern(env, "==");
        case Type::EqualEqualEqual:
            return SymbolValue::intern(env, "===");
        case Type::Exponent:
            return SymbolValue::intern(env, "**");
        case Type::ExponentEqual:
            return SymbolValue::intern(env, "**=");
        case Type::Float:
            return SymbolValue::intern(env, "float");
        case Type::GlobalVariable:
            return SymbolValue::intern(env, "gvar");
        case Type::GreaterThan:
            return SymbolValue::intern(env, ">");
        case Type::GreaterThanOrEqual:
            return SymbolValue::intern(env, ">=");
        case Type::HashRocket:
            return SymbolValue::intern(env, "=>");
        case Type::Identifier:
            return SymbolValue::intern(env, "identifier");
        case Type::InstanceVariable:
            return SymbolValue::intern(env, "ivar");
        case Type::Integer:
            return SymbolValue::intern(env, "integer");
        case Type::Invalid:
            env->raise("SyntaxError", "%d: syntax error, unexpected '%s'", m_line + 1, m_literal);
        case Type::Keyword:
            return SymbolValue::intern(env, "keyword");
        case Type::LBrace:
            return SymbolValue::intern(env, "{");
        case Type::LBracket:
            return SymbolValue::intern(env, "[");
        case Type::LParen:
            return SymbolValue::intern(env, "(");
        case Type::LeftShift:
            return SymbolValue::intern(env, "<<");
        case Type::LessThan:
            return SymbolValue::intern(env, "<");
        case Type::LessThanOrEqual:
            return SymbolValue::intern(env, "<=");
        case Type::Match:
            return SymbolValue::intern(env, "=~");
        case Type::Minus:
            return SymbolValue::intern(env, "-");
        case Type::MinusEqual:
            return SymbolValue::intern(env, "-=");
        case Type::Modulus:
            return SymbolValue::intern(env, "%");
        case Type::ModulusEqual:
            return SymbolValue::intern(env, "%=");
        case Type::Multiply:
            return SymbolValue::intern(env, "*");
        case Type::MultiplyEqual:
            return SymbolValue::intern(env, "*=");
        case Type::Not:
            return SymbolValue::intern(env, "!");
        case Type::NotEqual:
            return SymbolValue::intern(env, "!=");
        case Type::NotMatch:
            return SymbolValue::intern(env, "!~");
        case Type::Or:
            return SymbolValue::intern(env, "||");
        case Type::PercentLowerI:
            return SymbolValue::intern(env, "%i");
        case Type::PercentUpperI:
            return SymbolValue::intern(env, "%I");
        case Type::PercentLowerW:
            return SymbolValue::intern(env, "%w");
        case Type::PercentUpperW:
            return SymbolValue::intern(env, "%W");
        case Type::Plus:
            return SymbolValue::intern(env, "+");
        case Type::PlusEqual:
            return SymbolValue::intern(env, "+=");
        case Type::RBrace:
            return SymbolValue::intern(env, "}");
        case Type::RBracket:
            return SymbolValue::intern(env, "]");
        case Type::RParen:
            return SymbolValue::intern(env, ")");
        case Type::Regexp:
            return SymbolValue::intern(env, "regexp");
        case Type::RightShift:
            return SymbolValue::intern(env, ">>");
        case Type::SafeNavigation:
            return SymbolValue::intern(env, "&.");
        case Type::Semicolon:
            return SymbolValue::intern(env, ";");
        case Type::String:
            return SymbolValue::intern(env, "string");
        case Type::Symbol:
            return SymbolValue::intern(env, "symbol");
        case Type::SymbolKey:
            return SymbolValue::intern(env, "symbol_key");
        case Type::TernaryColon:
            return SymbolValue::intern(env, ":");
        case Type::TernaryQuestion:
            return SymbolValue::intern(env, "?");
        case Type::UnterminatedRegexp:
            env->raise("SyntaxError", "unterminated regexp meets end of file");
        case Type::UnterminatedString:
            env->raise("SyntaxError", "unterminated string meets end of file at line %i and column %i: %s", m_line, m_column, m_literal);
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
        case Type::Keyword:
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

    bool is_comment() { return m_type == Type::Comment; }
    bool is_eof() { return m_type == Type::Eof; }
    bool is_eol() { return m_type == Type::Eol; }
    bool is_semicolon() { return m_type == Type::Semicolon; }
    bool is_valid() { return m_type != Type::Invalid; }

    void validate(Env *env) {
        if (!is_valid()) {
            type_value(env);
            NAT_UNREACHABLE();
        }
    }

    // FIXME: make "def" its own type and eliminate this strcmp
    bool can_precede_method_name() {
        return m_type == Token::Type::Dot || m_type == Token::Type::ConstantResolution || (m_type == Token::Type::Keyword && strcmp(m_literal, "def") == 0);
    }

    size_t line() { return m_line; }
    size_t column() { return m_column; }

private:
    Type m_type { Type::Invalid };
    const char *m_literal { nullptr };
    nat_int_t m_integer { 0 };
    double m_double { 0 };
    size_t m_line { 0 };
    size_t m_column { 0 };
};

}
