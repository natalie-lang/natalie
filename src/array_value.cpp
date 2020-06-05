#include "natalie.hpp"

namespace Natalie {

ArrayValue::ArrayValue(Env *env)
    : Value { env, Value::Type::Array, const_get(env, NAT_OBJECT, "Array", true)->as_class() } {
    vector_init(&ary, 0);
}

}
