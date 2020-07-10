#include "natalie.hpp"
#include "natalie/builtin.hpp"
#include <math.h>

namespace Natalie {

Value *Float_eql(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    FloatValue *self = self_value->as_float();
    NAT_ASSERT_ARGC(1);
    Value *other = args[0];
    if (other->is_float() && other->as_float()->to_double() == self->to_double()) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Float_cmp(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    FloatValue *self = self_value->as_float();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    double d1 = self->to_double();
    double d2;
    if (NAT_TYPE(arg) == Value::Type::Float) {
        d2 = arg->as_float()->to_double();
    } else if (arg->respond_to(env, "coerce")) {
        Value *coerced = arg->send(env, "coerce", 1, &self_value, nullptr);
        d2 = (*coerced->as_array())[0]->as_float()->to_double();
    } else {
        NAT_RAISE(env, "ArgumentError", "comparison of Float with %s failed", arg->klass->class_name());
    }
    if (d1 < d2) {
        return new IntegerValue { env, -1 };
    } else if (d1 == d2) {
        return new IntegerValue { env, 0 };
    } else {
        return new IntegerValue { env, 1 };
    }
}

Value *Float_coerce(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    FloatValue *self = self_value->as_float();
    NAT_ASSERT_ARGC(1);
    ArrayValue *ary = new ArrayValue { env };
    ary->push(self);
    Value *arg = args[0];
    switch (NAT_TYPE(arg)) {
    case Value::Type::Float:
        ary->push(arg);
        break;
    case Value::Type::Integer:
        ary->push(new FloatValue { env, arg->as_integer()->to_int64_t() });
        break;
    case Value::Type::String:
        printf("TODO\n");
        abort();
        break;
    default:
        NAT_RAISE(env, "ArgumentError", "invalid value for Float(): %S", NAT_INSPECT(arg));
    }
    return ary;
}

Value *Float_to_i(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return new IntegerValue { env, static_cast<int64_t>(floor(self->as_float()->to_double())) };
}

Value *Float_nan(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    if (self->as_float()->is_nan()) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Float_div(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    FloatValue *self = self_value->as_float();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    NAT_ASSERT_TYPE(arg, Value::Type::Integer, "Integer");

    double dividend = self->to_double();
    double divisor;

    switch (arg->type) {
    case Value::Type::Integer:
        divisor = static_cast<double>(arg->as_integer()->to_int64_t());
        break;
    case Value::Type::Float:
        divisor = arg->as_float()->to_double();
        break;
    default:
        NAT_ASSERT_TYPE(arg, Value::Type::Integer, "Integer");
    }

    if (divisor == 0.0) {
        return FloatValue::nan(env);
    }
    double result = dividend / divisor;
    return new FloatValue { env, result };
}

}
