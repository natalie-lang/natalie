#include <algorithm>

#include "natalie.hpp"

namespace Natalie {

void ArrayValue::push_splat(Env *env, Value *val) {
    if (!val->is_array() && respond_to(env, val, "to_a")) {
        val = send(env, val, "to_a", 0, NULL, NULL);
    }
    if (val->is_array()) {
        for (Value *v : *val->as_array()) {
            push(*v);
        }
    } else {
        push(*val);
    }
}

Value *ArrayValue::pop(Env *env) {
    if (size() == 0) return NAT_NIL;
    Value *val = m_vector[m_vector.size() - 1];
    m_vector.set_size(m_vector.size() - 1);
    return val;
}

void ArrayValue::expand_with_nil(Env *env, ssize_t total) {
    for (ssize_t i = size(); i < total; i++) {
        push(*NAT_NIL);
    }
}

void ArrayValue::sort(Env *env) {
    auto cmp = [env](Value *a, Value *b) {
        Value *compare = send(env, a, "<=>", 1, &b, NULL);
        return compare->as_integer()->to_int64_t() < 0;
    };
    m_vector.sort(cmp);
}

}
