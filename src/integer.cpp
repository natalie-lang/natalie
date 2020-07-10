#include "natalie.hpp"
#include "natalie/builtin.hpp"
#include <math.h>

namespace Natalie {

Value *Integer_to_s(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IntegerValue *self = self_value->as_integer();
    NAT_ASSERT_ARGC(0);
    char buf[NAT_INT_64_MAX_BUF_LEN];
    int_to_string(self->to_int64_t(), buf);
    return new StringValue { env, buf };
}

Value *Integer_add(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IntegerValue *self = self_value->as_integer();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    NAT_ASSERT_TYPE(arg, Value::Type::Integer, "Integer");
    int64_t result = self->to_int64_t() + arg->as_integer()->to_int64_t();
    return new IntegerValue { env, result };
}

Value *Integer_sub(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IntegerValue *self = self_value->as_integer();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    NAT_ASSERT_TYPE(arg, Value::Type::Integer, "Integer");
    int64_t result = self->to_int64_t() - arg->as_integer()->to_int64_t();
    return new IntegerValue { env, result };
}

Value *Integer_mul(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IntegerValue *self = self_value->as_integer();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    NAT_ASSERT_TYPE(arg, Value::Type::Integer, "Integer");
    int64_t result = self->to_int64_t() * arg->as_integer()->to_int64_t();
    return new IntegerValue { env, result };
}

Value *Integer_div(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IntegerValue *self = self_value->as_integer();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    NAT_ASSERT_TYPE(arg, Value::Type::Integer, "Integer");

    int64_t dividend = self->to_int64_t();
    int64_t divisor = arg->as_integer()->to_int64_t();
    if (divisor == 0) {
        NAT_RAISE(env, "ZeroDivisionError", "divided by 0");
    }
    int64_t result = dividend / divisor;
    return new IntegerValue { env, result };
}

Value *Integer_mod(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IntegerValue *self = self_value->as_integer();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    NAT_ASSERT_TYPE(arg, Value::Type::Integer, "Integer");
    int64_t result = self->to_int64_t() % arg->as_integer()->to_int64_t();
    return new IntegerValue { env, result };
}

Value *Integer_pow(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IntegerValue *self = self_value->as_integer();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    NAT_ASSERT_TYPE(arg, Value::Type::Integer, "Integer");
    int64_t result = pow(self->to_int64_t(), arg->as_integer()->to_int64_t());
    return new IntegerValue { env, result };
}

Value *Integer_cmp(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IntegerValue *self = self_value->as_integer();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    if (NAT_TYPE(arg) != Value::Type::Integer) return NAT_NIL;
    int64_t i1 = self->to_int64_t();
    int64_t i2 = arg->as_integer()->to_int64_t();
    if (i1 < i2) {
        return new IntegerValue { env, -1 };
    } else if (i1 == i2) {
        return new IntegerValue { env, 0 };
    } else {
        return new IntegerValue { env, 1 };
    }
}

Value *Integer_eqeqeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IntegerValue *self = self_value->as_integer();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    if (NAT_TYPE(arg) == Value::Type::Integer && self->to_int64_t() == arg->as_integer()->to_int64_t()) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Integer_times(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IntegerValue *self = self_value->as_integer();
    NAT_ASSERT_ARGC(0);
    int64_t val = self->to_int64_t();
    assert(val >= 0);
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    Value *num;
    for (long long i = 0; i < val; i++) {
        num = new IntegerValue { env, i };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &num, nullptr);
    }
    return self;
}

Value *Integer_bitwise_and(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IntegerValue *self = self_value->as_integer();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    NAT_ASSERT_TYPE(arg, Value::Type::Integer, "Integer");
    return new IntegerValue { env, self->to_int64_t() & arg->as_integer()->to_int64_t() };
}

Value *Integer_bitwise_or(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IntegerValue *self = self_value->as_integer();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    NAT_ASSERT_TYPE(arg, Value::Type::Integer, "Integer");
    return new IntegerValue { env, self->to_int64_t() | arg->as_integer()->to_int64_t() };
}

Value *Integer_succ(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IntegerValue *self = self_value->as_integer();
    NAT_ASSERT_ARGC(0);
    return new IntegerValue { env, self->to_int64_t() + 1 };
}

Value *Integer_coerce(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IntegerValue *self = self_value->as_integer();
    NAT_ASSERT_ARGC(1);
    ArrayValue *ary = new ArrayValue { env };
    Value *arg = args[0];
    switch (NAT_TYPE(arg)) {
    case Value::Type::Float:
        // FIXME: narrowing conversion -- google this!
        ary->push(new FloatValue { env, (double)self->to_int64_t() });
        ary->push(arg);
        break;
    case Value::Type::Integer:
        ary->push(self);
        ary->push(arg);
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

Value *Integer_eql(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IntegerValue *self = self_value->as_integer();
    NAT_ASSERT_ARGC(1);
    Value *other = args[0];
    if (other->is_integer() && other->as_integer()->to_int64_t() == self->to_int64_t()) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

}
