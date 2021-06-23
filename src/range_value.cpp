#include "natalie.hpp"

namespace Natalie {

ValuePtr RangeValue::initialize(Env *env, ValuePtr begin, ValuePtr end, ValuePtr exclude_end_value) {
    m_begin = begin;
    m_end = end;
    m_exclude_end = exclude_end_value && exclude_end_value->is_truthy();
    return this;
}

ValuePtr RangeValue::to_a(Env *env) {
    ArrayValue *ary = new ArrayValue {};
    ValuePtr item = m_begin;
    if (m_begin.send(env, ">", 1, &m_end)->is_truthy())
        return this;
    if (m_exclude_end) {
        while (!item.send(env, "==", 1, &m_end)->is_truthy()) {
            ary->push(item);
            item = item.send(env, "succ");
        }
    } else {
        bool matches_end = false;
        do {
            matches_end = item.send(env, "==", 1, &m_end, nullptr)->is_truthy();
            ary->push(item);
            item = item.send(env, "succ");
        } while (!matches_end);
    }
    return ary;
}

ValuePtr RangeValue::each(Env *env, Block *block) {
    env->ensure_block_given(block);
    ValuePtr item = m_begin;
    if (m_begin.send(env, ">", 1, &m_end)->is_truthy())
        return this;
    if (m_exclude_end) {
        while (!item.send(env, "==", 1, &m_end)->is_truthy()) {
            NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
            item = item.send(env, "succ");
        }
    } else {
        bool matches_end = false;
        do {
            matches_end = item.send(env, "==", 1, &m_end, nullptr)->is_truthy();
            NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
            item = item.send(env, "succ");
        } while (!matches_end);
    }
    return this;
}

ValuePtr RangeValue::inspect(Env *env) {
    if (m_exclude_end) {
        return StringValue::format(env, "{}...{}", m_begin->inspect_str(env), m_end->inspect_str(env));
    } else {
        return StringValue::format(env, "{}..{}", m_begin->inspect_str(env), m_end->inspect_str(env));
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
            return TrueValue::the();
        }
    }
    return FalseValue::the();
}

ValuePtr RangeValue::eqeqeq(Env *env, ValuePtr arg) {
    if (m_begin->type() == Value::Type::Integer && arg->is_integer()) {
        // optimized path for integer ranges
        nat_int_t begin = m_begin->as_integer()->to_nat_int_t();
        nat_int_t end = m_end->as_integer()->to_nat_int_t();
        nat_int_t val = arg->as_integer()->to_nat_int_t();
        if (begin <= val && ((m_exclude_end && val < end) || (!m_exclude_end && val <= end))) {
            return TrueValue::the();
        }
    } else {
        if (m_exclude_end) {
            if (arg.send(env, ">=", 1, &m_begin)->is_truthy() && arg.send(env, "<", 1, &m_end)->is_truthy())
                return TrueValue::the();
        } else {
            if (arg.send(env, ">=", 1, &m_begin)->is_truthy() && arg.send(env, "<=", 1, &m_end)->is_truthy())
                return TrueValue::the();
        }
    }
    return FalseValue::the();
}

}
