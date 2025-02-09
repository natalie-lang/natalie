#include "natalie.hpp"
#include <json-c/json.h>

using namespace Natalie;

Value init_json(Env *env, Value self) {
    return NilObject::the();
}

static json_object *ruby_to_json(Env *env, Value input) {
    if (input.is_nil()) {
        return json_object_new_null();
    } else if (input.is_true()) {
        return json_object_new_boolean(true);
    } else if (input.is_false()) {
        return json_object_new_boolean(false);
    } else if (input.is_string()) {
        auto str = input.to_str(env);
        return json_object_new_string_len(str->c_str(), str->bytesize());
    } else {
        env->raise("ArgumentError", "Unable to parse input: {}", input.inspect_str(env));
    }
}

Value JSON_generate_inner(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 1);
    auto res = ruby_to_json(env, args[0]);
    auto json_string = json_object_to_json_string(res);
    auto string = new StringObject { json_string, Encoding::ASCII_8BIT };
    json_object_put(res);
    return string;
}
