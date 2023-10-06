#include "natalie.hpp"

using namespace Natalie;

Value defn_cpp_file(Env *env, Value self, Args args, Block *block) {
    return new StringObject { "cpp_file" };
}

Value init(Env *env, Value self) {
    self->define_method(env, "cpp_file"_s, defn_cpp_file, 0);
    return NilObject::the();
}
