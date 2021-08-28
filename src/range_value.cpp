#include "natalie.hpp"

namespace Natalie {

ValuePtr RangeValue::initialize(Env *env, ValuePtr begin, ValuePtr end, ValuePtr exclude_end_value) {
    m_begin = begin;
    m_end = end;
    m_exclude_end = exclude_end_value && exclude_end_value->is_truthy();
    return this;
}

template <typename Function>
ValuePtr RangeValue::iterate_over_range(Env *env, Function &&func) {
    if (m_begin.send(env, SymbolValue::intern(">"), 1, &m_end)->is_truthy())
        return nullptr;

    ValuePtr item = m_begin;

    auto eqeq = SymbolValue::intern("==");
    auto succ = SymbolValue::intern("succ");

    bool done = item.send(env, eqeq, 1, &m_end)->is_truthy();
    while (!done || !m_exclude_end) {
        if constexpr (std::is_void_v<std::invoke_result_t<Function, ValuePtr>>) {
            func(item);
        } else {
            if (ValuePtr ptr = func(item))
                return ptr;
        }

        if (!m_exclude_end && done) {
            break;
        }

        item = item.send(env, succ);

        done = item.send(env, eqeq, 1, &m_end)->is_truthy();
    }

    return nullptr;
}

ValuePtr RangeValue::to_a(Env *env) {
    ArrayValue *ary = new ArrayValue {};
    iterate_over_range(env, [&](ValuePtr item) {
        ary->push(item);
    });
    return ary;
}

ValuePtr RangeValue::each(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("each") });

    ValuePtr break_value = iterate_over_range(env, [&](ValuePtr item) -> ValuePtr {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        return nullptr;
    });
    if (break_value) {
        return break_value;
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
        bool begin_equal = m_begin.send(env, SymbolValue::intern("=="), 1, &begin, nullptr)->is_truthy();
        bool end_equal = m_end.send(env, SymbolValue::intern("=="), 1, &end, nullptr)->is_truthy();
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
            if (arg.send(env, SymbolValue::intern(">="), 1, &m_begin)->is_truthy() && arg.send(env, SymbolValue::intern("<"), 1, &m_end)->is_truthy())
                return TrueValue::the();
        } else {
            if (arg.send(env, SymbolValue::intern(">="), 1, &m_begin)->is_truthy() && arg.send(env, SymbolValue::intern("<="), 1, &m_end)->is_truthy())
                return TrueValue::the();
        }
    }
    return FalseValue::the();
}

ValuePtr RangeValue::include(Env *env, ValuePtr arg) {
    auto eqeq = SymbolValue::intern("==");
    ValuePtr found_item = iterate_over_range(env, [&](ValuePtr item) -> ValuePtr {
        if (arg.send(env, eqeq, 1, &item, nullptr)->is_truthy())
            return item;

        return nullptr;
    });
    if (found_item)
        return TrueValue::the();

    return FalseValue::the();
}

}
