#include "natalie.hpp"

namespace Natalie {

ArrayValue::ArrayValue(Env *env)
    : Value { env, Value::Type::Array, NAT_OBJECT->const_get(env, "Array", true)->as_class() } {
    vector_init(&ary, 0);
}

}
