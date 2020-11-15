#include "natalie.hpp"

namespace Natalie {

Value *RangeValue::initialize(Env *env, Value *begin, Value *end, Value *exclude_end_value) {
    m_begin = begin;
    m_end = end;
    m_exclude_end = exclude_end_value && exclude_end_value->is_truthy();
    return this;
}

Value *RangeValue::to_a(Env *env) {
    ArrayValue *ary = new ArrayValue { env };
    Value *item = m_begin;
    const char *op = m_exclude_end ? "<" : "<=";
    while (item->send(env, op, 1, &m_end, nullptr)->is_truthy()) {
        ary->push(item);
        item = item->send(env, "succ");
    }
    return ary;
}

Value *RangeValue::each(Env *env, Block *block) {
    env->assert_block_given(block);
    Value *item = m_begin;
    const char *op = m_exclude_end ? "<" : "<=";
    while (item->send(env, op, 1, &m_end, nullptr)->is_truthy()) {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        item = item->send(env, "succ");
    }
    return this;
}

Value *RangeValue::inspect(Env *env) {
    if (m_exclude_end) {
        return StringValue::sprintf(env, "%v...%v", m_begin, m_end);
    } else {
        return StringValue::sprintf(env, "%v..%v", m_begin, m_end);
    }
}

Value *RangeValue::eq(Env *env, Value *other_value) {
    if (other_value->is_range()) {
        RangeValue *other = other_value->as_range();
        Value *begin = other->begin();
        Value *end = other->end();
        bool begin_equal = m_begin->send(env, "==", 1, &begin, nullptr)->is_truthy();
        bool end_equal = m_end->send(env, "==", 1, &end, nullptr)->is_truthy();
        if (begin_equal && end_equal && m_exclude_end == other->m_exclude_end) {
            return env->true_obj();
        }
    }
    return env->false_obj();
}

Value *RangeValue::eqeqeq(Env *env, Value *arg) {
    if (m_begin->type() == Value::Type::Integer && arg->is_integer()) {
        // optimized path for integer ranges
        nat_int_t begin = m_begin->as_integer()->to_nat_int_t();
        nat_int_t end = m_end->as_integer()->to_nat_int_t();
        nat_int_t val = arg->as_integer()->to_nat_int_t();
        if (begin <= val && ((m_exclude_end && val < end) || (!m_exclude_end && val <= end))) {
            return env->true_obj();
        }
    } else {
        // slower method that should work for any type of range
        Value *item = m_begin;
        const char *op = m_exclude_end ? "<" : "<=";
        Value *end = m_end;
        while (item->send(env, op, 1, &end, nullptr)->is_truthy()) {
            if (item->send(env, "==", 1, &arg, nullptr)->is_truthy()) {
                return env->true_obj();
            }
            item = item->send(env, "succ");
        }
    }
    return env->false_obj();
}

}
