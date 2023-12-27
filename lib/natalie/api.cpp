#include "natalie.hpp"

using namespace Natalie;

extern void init_libnat(Env *env, Value self);

void init_api(Env *env, Value self) {
}

extern "C" {

Object *new_parser(StringObject *code, StringObject *path, ArrayObject *locals) {
    Env env {};
    try {
        auto Parser = fetch_nested_const({ "Natalie"_s, "Parser"_s });
        auto args_hash = new HashObject { &env, { "locals"_s, locals } };
        Value args[] = { code, path, args_hash };
        return Parser.send(&env, "new"_s, Args(3, args, true)).object();
    } catch (ExceptionObject *e) {
        dbg("{}", Value(e).send(&env, "message"_s));
        dbg("{}", Value(e).send(&env, "backtrace"_s).send(&env, "to_s"_s));
        return NilObject::the();
    }
}

Object *new_compiler(Object *ast, StringObject *path, EncodingObject *encoding) {
    Env env {};
    try {
        auto Compiler = fetch_nested_const({ "Natalie"_s, "Compiler"_s });
        auto args_hash = new HashObject { &env, { "ast"_s, ast, "path"_s, path, "encoding"_s, encoding } };
        Value args[] = { args_hash };
        return Compiler.send(&env, "new"_s, Args(1, args, true)).object();
    } catch (ExceptionObject *e) {
        dbg("{}", Value(e).send(&env, "message"_s));
        dbg("{}", Value(e).send(&env, "backtrace"_s).send(&env, "to_s"_s));
        return NilObject::the();
    }
}

// This is just here so we can have an extern "C" function to call.
void init_libnat2(Env *env, Object *self) {
    init_libnat(env, self);
}
}
