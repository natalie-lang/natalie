#include "natalie/enumerator/arithmetic_sequence_object.hpp"
#include "natalie.hpp"

namespace Natalie::Enumerator {
Integer ArithmeticSequenceObject::calculate_step_count(Env *env) {
    auto n = m_end->send(env, "-"_s, { m_begin })->send(env, "/"_s, { m_step });

    Integer step_count;
    if (n->is_integer()) {
        step_count = n->as_integer()->integer();
    } else {
        step_count = n->send(env, "+"_s, { n->send(env, "*"_s, { new FloatObject { std::numeric_limits<double>::epsilon() } }) })->send(env, "floor"_s)->as_integer()->integer();
    }

    if (exclude_end() && step_count > 0)
        step_count -= 1;

    return step_count;
}

Value ArithmeticSequenceObject::last(Env *env) {
    if (m_exclude_end) {
        auto steps = step_count(env);
        return m_begin->send(env, "+"_s, { IntegerObject::create(steps)->send(env, "*"_s, { m_step }) });
    } else {
        return m_end;
    }
}
};
