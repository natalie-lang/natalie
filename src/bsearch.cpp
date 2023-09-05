#include "natalie/bsearch.hpp"

namespace Natalie {
BSearchCheckResult binary_search_check(Env *env, Value block_result) {
    if (block_result->is_numeric()) {
        if (block_result->is_integer()) {
            auto i = block_result->as_integer();
            if (i->is_zero())
                return BSearchCheckResult::EQUAL;
            else if (i->is_negative())
                return BSearchCheckResult::SMALLER;
        } else {
            auto f = block_result->as_float();
            if (f->is_zero())
                return BSearchCheckResult::EQUAL;
            else if (f->is_negative())
                return BSearchCheckResult::SMALLER;
        }

        return BSearchCheckResult::BIGGER;
    } else if (block_result->is_boolean() || block_result->is_nil()) {
        if (block_result->is_true())
            return BSearchCheckResult::SMALLER;

        return BSearchCheckResult::BIGGER;
    } else {
        env->raise("TypeError", "wrong argument type {} (must be numeric, true, false or nil)", block_result->klass()->inspect_str());
    }
}

Optional<nat_int_t> binary_search(Env *env, nat_int_t left, nat_int_t right, std::function<Value(nat_int_t)> yielder) {
    Optional<nat_int_t> result;

    while (left <= right) {
        nat_int_t middle = (left < 0) == (right < 0) ? (left + (right - left) / 2) : ((right + left) / 2);
        auto block_result = yielder(middle);

        switch (binary_search_check(env, block_result)) {
        case BSearchCheckResult::EQUAL:
            return middle;
        case BSearchCheckResult::SMALLER: {
            if (!block_result->is_numeric())
                result = middle;
            right = middle - 1;
            break;
        }
        case BSearchCheckResult::BIGGER:
            left = middle + 1;
            break;
        }
    }
    return result;
}

Value binary_search_integer(Env *env, nat_int_t left, nat_int_t right, Block *block, bool exclude_end) {
    auto result = binary_search(env, left, right, [env, block](nat_int_t middle) -> Value {
        return NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, { IntegerObject::create(middle) }, nullptr);
    });

    if (!result.present())
        return NilObject::the();

    if (exclude_end && result.value() == right)
        return NilObject::the();

    return IntegerObject::create(result.value());
}

union double_as_integer {
    double d;
    nat_int_t i;
};

nat_int_t double_to_integer(double d) {
    union double_as_integer u;
    u.d = fabs(d);
    return d > 0 ? u.i : -u.i;
}

double integer_to_double(nat_int_t i) {
    union double_as_integer u;
    u.i = ::abs(i);
    return i > 0 ? u.d : -u.d;
}

Value binary_search_float(Env *env, double left, double right, Block *block, bool exclude_end) {
    nat_int_t left_int = double_to_integer(left);
    nat_int_t right_int = double_to_integer(right);

    auto result = binary_search(env, left_int, right_int, [env, block](nat_int_t middle) -> Value {
        return NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, { new FloatObject(integer_to_double(middle)) }, nullptr);
    });

    if (!result.present())
        return NilObject::the();

    if (exclude_end && result.value() == right_int)
        return NilObject::the();

    return new FloatObject { integer_to_double(result.value()) };
}

}
