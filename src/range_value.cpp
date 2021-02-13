#include "natalie.hpp"

namespace Natalie {

ValuePtr RangeValue::initialize(Env *env, ValuePtr begin, ValuePtr end, ValuePtr exclude_end_value) {
    m_begin = begin;
    m_end = end;
    m_exclude_end = exclude_end_value && exclude_end_value->is_truthy();
    return this;
}

ValuePtr RangeValue::to_a(Env *env) {
    ArrayValue *ary = new ArrayValue { env };
    ValuePtr item = m_begin;
    const char *op = m_exclude_end ? "<" : "<=";
    while (item.send(env, op, 1, &m_end, nullptr)->is_truthy()) {
        ary->push(item);
        item = item.send(env, "succ");
    }
    return ary;
}

ValuePtr RangeValue::each(Env *env, Block *block) {
    env->assert_block_given(block);
    ValuePtr item = m_begin;
    const char *op = m_exclude_end ? "<" : "<=";
    while (item.send(env, op, 1, &m_end, nullptr)->is_truthy()) {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        item = item.send(env, "succ");
    }
    return this;
}

ValuePtr RangeValue::inspect(Env *env) {
    if (m_exclude_end) {
        return StringValue::sprintf(env, "%v...%v", m_begin.value(), m_end.value());
    } else {
        return StringValue::sprintf(env, "%v..%v", m_begin.value(), m_end.value());
    }
}

ValuePtr RangeValue::eq(Env *env, ValuePtr other_value) {
    if (other_value->is_range()) {
        RangeValue *other = other_value->as_range();
        ValuePtr begin = other->begin();
        ValuePtr end = other->end();
        bool begin_equal = m_begin.send(env, "==", 1, &begin, nullptr)->is_truthy();
        bool end_equal = m_end.send(env, "==", 1, &end, nullptr)->is_truthy();
        if (begin_equal && end_equal && m_exclude_end == other->m_exclude_end) {
            return env->true_obj();
        }
    }
    return env->false_obj();
}

ValuePtr RangeValue::eqeqeq(Env *env, ValuePtr arg) {
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
        ValuePtr item = m_begin;
        const char *op = m_exclude_end ? "<" : "<=";
        ValuePtr end = m_end;
        while (item.send(env, op, 1, &end, nullptr)->is_truthy()) {
            if (item.send(env, "==", 1, &arg, nullptr)->is_truthy()) {
                return env->true_obj();
            }
            item = item.send(env, "succ");
        }
    }
    return env->false_obj();
}

}
