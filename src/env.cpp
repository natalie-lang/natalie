#include <stdarg.h>

#include "natalie.hpp"
#include "natalie/string.hpp"

namespace Natalie {

Env Env::new_detatched_env(Env *outer) {
    return Env { outer->global_env() };
}

void Env::build_vars(size_t size) {
    m_vars = std::make_shared<Vector<ValuePtr>>(size, nil_obj());
}

ValuePtr Env::global_get(SymbolValue *name) {
    return m_global_env->global_get(this, name);
}

ValuePtr Env::global_set(SymbolValue *name, ValuePtr val) {
    return m_global_env->global_set(this, name, val);
}

Method *Env::current_method() {
    Env *env = this;
    while (!env->method() && env->outer()) {
        env = env->outer();
    }
    return env->method();
}

char *Env::build_code_location_name(Env *location_env) {
    if (location_env->is_main())
        return GC_STRDUP("<main>");
    if (location_env->method())
        return GC_STRDUP(location_env->method()->name());
    if (location_env->outer()) {
        char *outer_name = build_code_location_name(location_env->outer());
        char *name = GC_STRDUP(StringValue::format(this, "block in {}", outer_name)->c_str());
        GC_FREE(outer_name);
        return name;
    }
    return GC_STRDUP("block");
}

void Env::raise(ClassValue *klass, StringValue *message) {
    ExceptionValue *exception = new ExceptionValue { this, klass, message };
    this->raise_exception(exception);
}

void Env::raise(ClassValue *klass, struct String *message) {
    ExceptionValue *exception = new ExceptionValue { this, klass, new StringValue { this, *message } };
    this->raise_exception(exception);
}

void Env::raise(const char *class_name, struct String *message) {
    ClassValue *klass = Object()->const_fetch(this, SymbolValue::intern(this, class_name))->as_class();
    ExceptionValue *exception = new ExceptionValue { this, klass, new StringValue { this, *message } };
    this->raise_exception(exception);
}

void Env::raise_exception(ExceptionValue *exception) {
    if (!exception->backtrace()) {
        // only build a backtrace the first time the exception is raised (not on a re-raise)
        exception->build_backtrace(this);
    }
    throw exception;
}

void Env::raise_local_jump_error(ValuePtr exit_value, const char *message) {
    ExceptionValue *exception = new ExceptionValue { this, Object()->const_find(this, SymbolValue::intern(this, "LocalJumpError"))->as_class(), new StringValue { this, message } };
    exception->ivar_set(this, SymbolValue::intern(this, "@exit_value"), exit_value);
    this->raise_exception(exception);
}

void Env::raise_errno() {
    ValuePtr exception_args[] = { ValuePtr { this, errno } };
    ExceptionValue *error = Object()->const_find(this, SymbolValue::intern(this, "SystemCallError")).send(this, "exception", 1, exception_args, nullptr)->as_exception();
    raise_exception(error);
}

void Env::assert_argc(size_t argc, size_t expected) {
    if (argc != expected) {
        raise("ArgumentError", "wrong number of arguments (given {}, expected {})", argc, expected);
    }
}

void Env::assert_argc(size_t argc, size_t expected_low, size_t expected_high) {
    if (argc < expected_low || argc > expected_high) {
        raise("ArgumentError", "wrong number of arguments (given {}, expected {}..{})", argc, expected_low, expected_high);
    }
}

void Env::assert_argc_at_least(size_t argc, size_t expected) {
    if (argc < expected) {
        raise("ArgumentError", "wrong number of arguments (given {}, expected {}+)", argc, expected);
    }
}

void Env::assert_block_given(Block *block) {
    if (!block) {
        raise("ArgumentError", "called without a block");
    }
}

ValuePtr Env::last_match() {
    if (m_match) {
        return m_match;
    } else {
        return nil_obj();
    }
}

ValuePtr Env::var_get(const char *key, size_t index) {
    if (index >= m_vars->size()) {
        printf("Trying to get variable `%s' at index %zu which is not set.\n", key, index);
        abort();
    }
    ValuePtr val = (*m_vars)[index];
    if (val) {
        return val;
    } else {
        return nil_obj();
    }
}

ValuePtr Env::var_set(const char *name, size_t index, bool allocate, ValuePtr val) {
    size_t needed = index + 1;
    size_t current_size = m_vars ? m_vars->size() : 0;
    if (needed > current_size) {
        if (allocate) {
            if (!m_vars) {
                m_vars = std::make_shared<Vector<ValuePtr>>(needed, nil_obj());
            } else {
                m_vars->set_size(needed, nil_obj());
            }
        } else {
            printf("Tried to set a variable without first allocating space for it.\n");
            abort();
        }
    }
    (*m_vars)[index] = val;
    return val;
}

void Env::visit_children(Visitor &visitor) {
    visitor.visit(m_global_env);
    if (m_vars) {
        for (auto &val : *m_vars) {
            visitor.visit(val);
        }
    }
    visitor.visit(m_outer);
    visitor.visit(m_block);
    visitor.visit(m_caller);
    visitor.visit(m_method);
    visitor.visit(m_match);
}

}
