#include "natalie.hpp"
#include "natalie/array_object.hpp"
#include "natalie/env.hpp"
#include "natalie/natalie_parser/natalie_creator.hpp"
#include "natalie/symbol_object.hpp"
#include "natalie_parser/parser.hpp"

namespace Natalie {

Value ParserObject::parse(Env *env, Value code, Value source_path) {
    code->assert_type(env, Object::Type::String, "String");
    if (source_path)
        source_path->assert_type(env, Object::Type::String, "String");
    else
        source_path = new StringObject { "(string)" };
    code->assert_type(env, Object::Type::String, "String");
    auto parser = NatalieParser::Parser {
        new String(code->as_string()->string()),
        new String(source_path->as_string()->string())
    };
    SharedPtr<NatalieParser::Node> tree;
    try {
        tree = parser.tree();
    } catch (NatalieParser::Parser::SyntaxError &e) {
        env->raise("SyntaxError", e.message());
    }
    auto ast = node_to_ruby(env, *tree);
    return ast;
}

Value ParserObject::tokens(Env *env, Value code, Value with_line_and_column_numbers) {
    code->assert_type(env, Object::Type::String, "String");
    auto lexer = NatalieParser::Lexer {
        new String(code->as_string()->string()),
        new String("(string)")
    };
    auto array = new ArrayObject {};
    auto the_tokens = lexer.tokens();
    auto include_line_and_column_numbers = with_line_and_column_numbers && with_line_and_column_numbers->is_truthy();
    for (auto token : *the_tokens) {
        auto token_value = token_to_ruby(env, token, include_line_and_column_numbers);
        if (token_value->is_truthy())
            array->push(token_value);
    }
    return array;
}

Value ParserObject::token_to_ruby(Env *env, NatalieParser::Token &token, bool with_line_and_column_numbers) {
    if (token.is_eof())
        return NilObject::the();
    validate_token(env, token);
    const char *type = token.type_value();
    auto hash = new HashObject {};
    hash->put(env, "type"_s, SymbolObject::intern(type));
    switch (token.type()) {
    case NatalieParser::Token::Type::Bignum:
    case NatalieParser::Token::Type::PercentLowerI:
    case NatalieParser::Token::Type::PercentUpperI:
    case NatalieParser::Token::Type::PercentLowerW:
    case NatalieParser::Token::Type::PercentUpperW:
    case NatalieParser::Token::Type::String:
        hash->put(env, "literal"_s, new StringObject { token.literal_or_blank() });
        break;
    case NatalieParser::Token::Type::BareName:
    case NatalieParser::Token::Type::ClassVariable:
    case NatalieParser::Token::Type::Constant:
    case NatalieParser::Token::Type::GlobalVariable:
    case NatalieParser::Token::Type::InstanceVariable:
    case NatalieParser::Token::Type::Symbol:
    case NatalieParser::Token::Type::SymbolKey:
        hash->put(env, "literal"_s, SymbolObject::intern(token.literal_or_blank()));
        break;
    case NatalieParser::Token::Type::Float:
        hash->put(env, "literal"_s, new FloatObject { token.get_double() });
        break;
    case NatalieParser::Token::Type::Fixnum:
        hash->put(env, "literal"_s, Value::integer(token.get_fixnum()));
        break;
    case NatalieParser::Token::Type::InterpolatedRegexpEnd:
        if (token.has_literal())
            hash->put(env, "options"_s, new StringObject { token.literal_string().ref() });
        break;
    default:
        void();
    }
    if (with_line_and_column_numbers) {
        hash->put(env, "line"_s, Value::integer(token.line()));
        hash->put(env, "column"_s, Value::integer(token.column()));
    }
    return hash;
}

void ParserObject::validate_token(Env *env, NatalieParser::Token &token) {
    try {
        token.validate();
    } catch (NatalieParser::Parser::SyntaxError &e) {
        env->raise("SyntaxError", e.message());
    }
}

Value ParserObject::node_to_ruby(Env *env, const NatalieParser::Node &node) {
    NatalieCreator creator { env };
    node.transform(&creator);
    return creator.sexp();
}

}
