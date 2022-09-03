#include "natalie.hpp"

using namespace Natalie;

Value defn_foo5(Env *env, Value self, Args args, Block *block) {
    return new StringObject { "foo5" };
}

Value init(Env *env, Value self) {
    self->define_method(env, "foo5"_s, defn_foo5, 0);
    return NilObject::the();
}
