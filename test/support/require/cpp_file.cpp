#include "natalie.hpp"

using namespace Natalie;

Value defn_cpp_file(Env *env, Value self, Args &&, Block *block) {
    return StringObject::create("cpp_file");
}

Value init_cpp_file(Env *env, Value self) {
    Object::define_method(env, self, "cpp_file"_s, defn_cpp_file, 0);
    return Value::nil();
}
