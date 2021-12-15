#include "natalie.hpp"

namespace Natalie {

ValuePtr RangeObject::initialize(Env *env, ValuePtr begin, ValuePtr end, ValuePtr exclude_end_value) {
    m_begin = begin;
    m_end = end;
    m_exclude_end = exclude_end_value && exclude_end_value->is_truthy();
    return this;
}

template <typename Function>
ValuePtr RangeObject::iterate_over_range(Env *env, Function &&func) {
    if (m_begin.send(env, SymbolObject::intern(">"), { m_end })->is_truthy())
        return nullptr;

    ValuePtr item = m_begin;

    auto eqeq = SymbolObject::intern("==");
    auto succ = SymbolObject::intern("succ");

    bool done = item.send(env, eqeq, { m_end })->is_truthy();
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

        done = item.send(env, eqeq, { m_end })->is_truthy();
    }

    return nullptr;
}

ValuePtr RangeObject::to_a(Env *env) {
    ArrayObject *ary = new ArrayObject {};
    iterate_over_range(env, [&](ValuePtr item) {
        ary->push(item);
    });
    return ary;
}

ValuePtr RangeObject::each(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolObject::intern("enum_for"), { SymbolObject::intern("each") });

    ValuePtr break_value = iterate_over_range(env, [&](ValuePtr item) -> ValuePtr {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        return nullptr;
    });
    if (break_value) {
        return break_value;
    }

    return this;
}

ValuePtr RangeObject::first(Env *env, ValuePtr n) {
    if (n) {
        if (n->respond_to(env, SymbolObject::intern("to_int"))) {
            n = n->send(env, SymbolObject::intern("to_int"));
        }
        n->assert_type(env, Object::Type::Integer, "Integer");

        nat_int_t count = n->as_integer()->to_nat_int_t();
        if (count < 0) {
            env->raise("ArgumentError", "negative array size (or size too big)");
            return nullptr;
        }

        ArrayObject *ary = new ArrayObject {};
        iterate_over_range(env, [&](ValuePtr item) -> ValuePtr {
            if (count == 0) return n;

            ary->push(item);
            count--;
            return nullptr;
        });

        return ary;
    } else {
        return begin();
    }
}

ValuePtr RangeObject::inspect(Env *env) {
    if (m_exclude_end) {
        return StringObject::format(env, "{}...{}", m_begin->inspect_str(env), m_end->inspect_str(env));
    } else {
        return StringObject::format(env, "{}..{}", m_begin->inspect_str(env), m_end->inspect_str(env));
    }
}

ValuePtr RangeObject::eq(Env *env, ValuePtr other_value) {
    if (other_value->is_range()) {
        RangeObject *other = other_value->as_range();
        ValuePtr begin = other->begin();
        ValuePtr end = other->end();
        bool begin_equal = m_begin.send(env, SymbolObject::intern("=="), { begin })->is_truthy();
        bool end_equal = m_end.send(env, SymbolObject::intern("=="), { end })->is_truthy();
        if (begin_equal && end_equal && m_exclude_end == other->m_exclude_end) {
            return TrueObject::the();
        }
    }
    return FalseObject::the();
}

ValuePtr RangeObject::eqeqeq(Env *env, ValuePtr arg) {
    if (m_begin->type() == Object::Type::Integer && arg->is_integer()) {
        // optimized path for integer ranges
        nat_int_t begin = m_begin->as_integer()->to_nat_int_t();
        nat_int_t end = m_end->as_integer()->to_nat_int_t();
        nat_int_t val = arg->as_integer()->to_nat_int_t();
        if (begin <= val && ((m_exclude_end && val < end) || (!m_exclude_end && val <= end))) {
            return TrueObject::the();
        }
    } else {
        if (m_exclude_end) {
            if (arg.send(env, SymbolObject::intern(">="), { m_begin })->is_truthy() && arg.send(env, SymbolObject::intern("<"), 1, &m_end)->is_truthy())
                return TrueObject::the();
        } else {
            if (arg.send(env, SymbolObject::intern(">="), { m_begin })->is_truthy() && arg.send(env, SymbolObject::intern("<="), 1, &m_end)->is_truthy())
                return TrueObject::the();
        }
    }
    return FalseObject::the();
}

ValuePtr RangeObject::include(Env *env, ValuePtr arg) {
    auto eqeq = SymbolObject::intern("==");
    ValuePtr found_item = iterate_over_range(env, [&](ValuePtr item) -> ValuePtr {
        if (arg.send(env, eqeq, { item })->is_truthy())
            return item;

        return nullptr;
    });
    if (found_item)
        return TrueObject::the();

    return FalseObject::the();
}

}
