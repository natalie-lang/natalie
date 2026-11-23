#include "natalie.hpp"

using namespace Natalie;

Value init_gc_helpers(Env *env, Value self) { return Value::nil(); }

void GC_create_probe() {
    VoidPObject::create(nullptr, [](auto p) {
        Env env {};
        GlobalEnv::the()->global_set(&env, "$gc_ran"_s, Value::True());
    });
}
