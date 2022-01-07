#pragma once

#include <memory>
#include <stdlib.h>

#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/global_env.hpp"
#include "natalie/local_jump_error_type.hpp"
#include "natalie/managed_vector.hpp"
#include "natalie/string.hpp"
#include "natalie/value.hpp"
#include "tm/shared_ptr.hpp"

namespace Natalie {

using namespace TM;

class Env : public Cell {
public:
    Env() { }

    Env(Env *outer)
        : m_outer { outer } { }

    Env(const Env &other)
        : m_vars { other.m_vars }
        , m_outer { other.m_outer }
        , m_block { other.m_block }
        , m_caller { nullptr }
        , m_file { other.m_file }
        , m_line { other.m_line }
        , m_method { other.m_method } { }

    Env &operator=(Env &other) = delete;

    virtual ~Env() override { }

    Value global_get(SymbolObject *);
    Value global_set(SymbolObject *, Value);

    Method *current_method();
    const String *build_code_location_name(Env *);

    Value var_get(const char *, size_t);
    Value var_set(const char *, size_t, bool, Value);

    [[noreturn]] void raise(ClassObject *, StringObject *);
    [[noreturn]] void raise(ClassObject *, const String *);
    [[noreturn]] void raise(const char *, const String *);
    [[noreturn]] void raise_exception(ExceptionObject *);
    [[noreturn]] void raise_key_error(Value, Value);
    [[noreturn]] void raise_local_jump_error(Value, LocalJumpErrorType);
    [[noreturn]] void raise_errno();
    [[noreturn]] void raise_name_error(SymbolObject *name, const String *);

    template <typename... Args>
    [[noreturn]] void raise_name_error(SymbolObject *name, const char *format, Args... args) {
        auto message = String::format(format, args...);
        raise_name_error(name, message);
    }

    template <typename... Args>
    [[noreturn]] void raise(ClassObject *klass, const char *format, Args... args) {
        auto message = String::format(format, args...);
        raise(klass, message);
    }

    template <typename... Args>
    [[noreturn]] void raise(const char *class_name, const char *format, Args... args) {
        auto message = String::format(format, args...);
        raise(class_name, message);
    }

    void warn(const String *);

    template <typename... Args>
    void warn(const char *format, Args... args) {
        auto message = String::format(format, args...);
        warn(message);
    }

    void ensure_argc_is(size_t argc, size_t expected);
    void ensure_argc_between(size_t argc, size_t expected_low, size_t expected_high);
    void ensure_argc_at_least(size_t argc, size_t expected);
    void ensure_block_given(Block *);

    Value last_match();
    bool has_last_match();
    void set_last_match(Value match);

    void build_vars(size_t);

    Env *outer() { return m_outer; }
    void clear_outer() { m_outer = nullptr; }

    Env *caller() { return m_caller; }
    void set_caller(Env *caller) { m_caller = caller; }

    Env *non_block_env();

    Block *block() { return m_block; }
    void set_block(Block *block) { m_block = block; }

    Block *this_block() { return m_this_block; }
    void set_this_block(Block *block) { m_this_block = block; }

    const char *file() { return m_file; }
    void set_file(const char *file) { m_file = file; }

    size_t line() const { return m_line; }
    void set_line(size_t line) { m_line = line; }

    Method *method() { return m_method; }
    void set_method(Method *method) { m_method = method; }

    Value match() { return m_match; }
    void set_match(Value match) { m_match = match; }
    void clear_match() { m_match = nullptr; }

    bool is_main() { return this == GlobalEnv::the()->main_env(); }

    virtual void visit_children(Visitor &visitor) override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<Env %p>", this);
    }

private:
    ManagedVector<Value> *m_vars { nullptr };
    Env *m_outer { nullptr };
    Block *m_block { nullptr };
    Block *m_this_block { nullptr };
    Env *m_caller { nullptr };
    const char *m_file { nullptr };
    size_t m_line { 0 };
    Method *m_method { nullptr };
    Value m_match { nullptr };
};
}
