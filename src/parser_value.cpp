#include "natalie.hpp"
#include "natalie/array_value.hpp"
#include "natalie/env.hpp"
#include "natalie/symbol_value.hpp"

#include <peg_parser/generator.h>

namespace Natalie {

using std::string;

Value *ParserValue::parse(Env *env, Value *code) {
    peg_parser::ParserGenerator<Value *, Env *&> g;

    g.setSeparator(g["Whitespace"] << "[\t ]");
    g["Operator"] << "[+\\-\\*/]" >> [](auto e, Env *env) { return SymbolValue::intern(env, e.string().c_str()); };
    g["Op"] << "Numeric Operator Numeric" >> [](auto e, Env *env) { return new ArrayValue { env, { SymbolValue::intern(env, "call"), e[0].evaluate(env), e[1].evaluate(env), e[2].evaluate(env) } }; };
    g["Float"] << "'-'? [0-9]+ '.' [0-9]+" >> [](auto e, Env *env) { return new ArrayValue { env, { SymbolValue::intern(env, "lit"), new FloatValue { env, stof(e.string()) } } }; };
    g["Integer"] << "'-'? [0-9]+" >> [](auto e, Env *env) { return new ArrayValue { env, { SymbolValue::intern(env, "lit"), new IntegerValue { env, stoll(e.string()) } } }; };
    g["Numeric"] << "Float | Integer";
    g["EscapedChar"] << "'\\\\' .";
    g["DqChar"] << "!'\"' .";
    g["DqString"] << "'\"' (EscapedChar | DqChar)* '\"'" >> [](auto e, Env *env) {
        string result;
        string s = e.string();
        for (size_t i = 1; i < s.length() - 1; i++) {
            char c = s[i];
            if (c == '\\') {
                i++;
                c = s[i];
                switch (c) {
                case 'n':
                    result += '\n';
                    break;
                case 't':
                    result += '\t';
                    break;
                default:
                    result += c;
                }
                continue;
            }
            result += c;
        }
        return new ArrayValue { env, { SymbolValue::intern(env, "str"), new StringValue { env, result.c_str() } } };
    };
    g["SqChar"] << "!'\\'' .";
    g["SqString"] << "'\\'' (EscapedChar | SqChar)* '\\''" >> [](auto e, Env *env) {
        string result;
        string s = e.string();
        for (size_t i = 1; i < s.length() - 1; i++) {
            char c = s[i];
            if (c == '\\') {
                i++;
                c = s[i];
                switch (c) {
                case '\\':
                case '\'':
                    result += c;
                    break;
                default:
                    result += '\\';
                    result += c;
                }
                continue;
            }
            result += c;
        }
        return new ArrayValue { env, { SymbolValue::intern(env, "str"), new StringValue { env, result.c_str() } } };
    };
    g["Expression"] << "Op | DqString | SqString | Numeric";
    g.setStart(g["Expression"]);

    const char *input = code->as_string()->c_str();
    Value *result;
    try {
        result = g.run(input, env);
    } catch (peg_parser::SyntaxError &error) {
        // TODO: return message about what went wrong, probably using the following stuff:
        //printf("begin = %zu\n", error.syntax->begin);
        //printf("length = %zu\n", error.syntax->length());
        //printf("rule = %s\n", error.syntax->rule->name.c_str());
        NAT_RAISE(env, "SyntaxError", "syntax error");
    }
    Value *block = new ArrayValue { env, { SymbolValue::intern(env, "block"), result } };
    return block;
}

}
