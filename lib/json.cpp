#include "natalie.hpp"
#include <json-c/json.h>

using namespace Natalie;

Value init_json(Env *env, Value self) {
    return NilObject::the();
}

Value JSON_generate_bool(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 1);
    auto res = json_object_new_boolean(args[0].is_truthy());
    auto json_string = json_object_to_json_string(res);
    auto string = new StringObject { json_string, Encoding::ASCII_8BIT };
    json_object_put(res);
    return string;
}

Value JSON_generate_nil(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);
    auto res = json_object_new_null();
    auto json_string = json_object_to_json_string(res);
    auto string = new StringObject { json_string, Encoding::ASCII_8BIT };
    json_object_put(res);
    return string;
}

Value JSON_generate_string(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 1);
    auto str = args[0].to_str(env);
    auto res = json_object_new_string_len(str->c_str(), str->bytesize());
    auto json_string = json_object_to_json_string(res);
    auto string = new StringObject { json_string, Encoding::ASCII_8BIT };
    json_object_put(res);
    return string;
}
