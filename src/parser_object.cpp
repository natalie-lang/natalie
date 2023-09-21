#include "natalie.hpp"
#include "natalie/array_object.hpp"
#include "natalie/env.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

Value ParserObject::parse(Env *env, Value code, Value source_path) {
    /*
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
    */
    return nullptr;
}

Value ParserObject::tokens(Env *env, Value code, Value with_line_and_column_numbers) {
    /*
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
    */
    return nullptr;
}

}
