#include "natalie.hpp"
#include "natalie/array_value.hpp"
#include "natalie/env.hpp"
#include "natalie/lexer.hpp"
#include "natalie/parser.hpp"
#include "natalie/symbol_value.hpp"

namespace Natalie {

using std::string;

Value *ParserValue::parse(Env *env, Value *code) {
    code->assert_type(env, Value::Type::String, "String");
    auto parser = Parser { code->as_string()->c_str() };
    return parser.tree(env)->to_ruby(env);
}

Value *ParserValue::tokens(Env *env, Value *code, Value *with_line_and_column_numbers) {
    auto lexer = Lexer { code->as_string()->c_str() };
    auto array = new ArrayValue { env };
    auto the_tokens = lexer.tokens();
    auto include_line_and_column_numbers = with_line_and_column_numbers && with_line_and_column_numbers->is_truthy();
    for (auto token : *the_tokens) {
        auto token_value = token.to_ruby(env, include_line_and_column_numbers);
        if (token_value->is_truthy())
            array->push(token_value);
    }
    return array;
}

}
