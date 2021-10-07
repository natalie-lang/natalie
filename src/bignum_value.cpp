#include "natalie.hpp"

namespace Natalie {
ValuePtr BignumValue::to_s(Env *env, ValuePtr base_value) {
    if (base_value) {
        // NATFIXME
        NAT_UNREACHABLE();
    }
    return new StringValue { m_bignum->to_string().c_str() };
}

ValuePtr BignumValue::add(Env *env, ValuePtr arg) {
    if (arg->is_float()) {
        double current = strtod(m_bignum->to_string().c_str(), NULL);
        auto result = current + arg->as_float()->to_double();
        return new FloatValue { result };
    }
    if (!arg->is_integer() && arg->respond_to(env, SymbolValue::intern("coerce"))) {
        auto array = arg->send(env, SymbolValue::intern("coerce"), { this });
        arg = array->as_array()->at(1);
    }
    arg.assert_type(env, Value::Type::Integer, "Integer");

    auto other = arg->as_integer();
    return new BignumValue { to_bignum() + other->to_bignum() };
}
}
