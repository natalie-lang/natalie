#include "natalie.hpp"

namespace Natalie {

Value Method::call(Env *env, Value self, Args args, Block *block) const {
    assert(m_fn);

    Env *closure_env;
    if (has_env()) {
        closure_env = m_env;
    } else {
        closure_env = m_owner->env();
    }
    Env e { closure_env };
    e.set_caller(env);
    e.set_method(this);
    e.set_file(env->file());
    e.set_line(env->line());
    e.set_block(block);

    auto call_fn = [&](Args args) {
        if (block && !block->calling_env()) {
            Defer clear_calling_env([&]() {
                block->clear_calling_env();
            });
            block->set_calling_env(env);
            return m_fn(&e, self, args, block);
        } else {
            return m_fn(&e, self, args, block);
        }
    };

    // This code handles the "fast" integer/float optimization, where certain
    // IntegerObject and FloatObject methods do not allow their `this` or their
    // arguments to escape outside their call stack, i.e. they only live for a
    // short period. Thus the objects can be stack-allocated for speed, and the
    // GC need not allocate or collect them.
    if (m_optimized) {
        if (args.size() == 1 && args[0].is_fast_integer()) {
            auto synthesized_arg = IntegerObject { args[0].get_fast_integer() };
            synthesized_arg.add_synthesized_flag();
            Value new_args[] = { &synthesized_arg };
            return call_fn(Args(1, new_args));
        } else if (args.size() == 1 && args[0].holds_raw_double()) {
            auto synthesized_arg = FloatObject { args[0].as_double() };
            synthesized_arg.add_synthesized_flag();
            Value new_args[] = { &synthesized_arg };
            return call_fn(Args(1, new_args));
        }
    } else if (self->is_synthesized()) {
        // Turn this object into a heap-allocated one.
        self = self->dup(env);
    }

    return call_fn(args);
}
}
