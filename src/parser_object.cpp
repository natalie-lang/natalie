#include "natalie.hpp"
#include "natalie/array_object.hpp"
#include "natalie/env.hpp"
#include "natalie/lexer.hpp"
#include "natalie/parser.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

ValuePtr ParserObject::parse(Env *env, ValuePtr code, ValuePtr source_path) {
    code->assert_type(env, Object::Type::String, "String");
    if (source_path)
        source_path->assert_type(env, Object::Type::String, "String");
    else
        source_path = new StringObject { "(string)" };
    code->assert_type(env, Object::Type::String, "String");
    auto parser = Parser { code->as_string()->to_low_level_string(), source_path->as_string()->to_low_level_string() };
    Node *tree;
    try {
        tree = parser.tree();
    } catch (Parser::SyntaxError &e) {
        env->raise("SyntaxError", e.message());
    }
    return tree->to_ruby(env);
}

ValuePtr ParserObject::tokens(Env *env, ValuePtr code, ValuePtr with_line_and_column_numbers) {
    code->assert_type(env, Object::Type::String, "String");
    auto lexer = Lexer { code->as_string()->to_low_level_string(), new String("(string)") };
    auto array = new ArrayObject {};
    auto the_tokens = lexer.tokens();
    auto include_line_and_column_numbers = with_line_and_column_numbers && with_line_and_column_numbers->is_truthy();
    for (auto token : *the_tokens) {
        auto token_value = token->to_ruby(env, include_line_and_column_numbers);
        if (token_value->is_truthy())
            array->push(token_value);
    }
    return array;
}

}
