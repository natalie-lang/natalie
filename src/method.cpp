#include "natalie.hpp"

namespace Natalie {

Value Method::call(Env *env, Value self, Args &&args, Block *block) const {
    assert(m_fn);

    Env *closure_env = nullptr;
    if (has_env())
        closure_env = m_env;
    Env e { closure_env };
    e.set_caller(env);
    e.set_method(this);
    e.set_lexical_scope(m_lexical_scope);
    e.set_file(env->file());
    e.set_line(env->line());
    e.set_block(block);

    if (m_self)
        self = m_self.value();

    auto call_fn = [&](Args &&args) {
        if (block && !block->calling_env()) {
            Defer clear_calling_env([&]() {
                block->clear_calling_env();
            });
            block->set_calling_env(env);
            return m_fn(&e, self, std::move(args), block);
        } else {
            return m_fn(&e, self, std::move(args), block);
        }
    };

    // For a block converted to a method, catch any LocalJumpErrorType::Return
    if (m_break_point == -1) {
        try {
            return call_fn(std::move(args));
        } catch (ExceptionObject *exception) {
            if (exception->local_jump_error_type() == LocalJumpErrorType::Return)
                return exception->send(env, "exit_value"_s);
            throw exception;
        }
    }

    // For a regular method with a non-local jump, catch a LocalJumpError with matching breakpoint
    if (m_break_point > 0) {
        try {
            return call_fn(std::move(args));
        } catch (ExceptionObject *exception) {
            if (exception->is_local_jump_error_with_break_point(m_break_point))
                return exception->send(env, "exit_value"_s);
            throw exception;
        }
    }

    return call_fn(std::move(args));
}
}
