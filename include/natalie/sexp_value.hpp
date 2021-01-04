#pragma once

#include "natalie.hpp"

namespace Natalie {

struct SexpValue : ArrayValue {
    SexpValue(Env *, Node *, std::initializer_list<Value *>);

    Value *new_method(Env *env, size_t argc, Value **args) {
        auto sexp = new SexpValue { env, {} };
        sexp->m_file = m_file;
        sexp->m_line = m_line;
        for (size_t i = 0; i < argc; i++) {
            sexp->push(args[i]);
        }
        return sexp;
    }

    Value *inspect(Env *env) {
        StringValue *out = new StringValue { env, "s(" };
        for (size_t i = 0; i < size(); i++) {
            Value *obj = (*this)[i];
            StringValue *repr = obj->send(env, "inspect")->as_string();
            out->append_string(env, repr);
            if (i < size() - 1) {
                out->append(env, ", ");
            }
        }
        out->append_char(env, ')');
        return out;
    }

    const char *file() { return m_file; }
    void set_file(const char *file) { m_file = file; }
    Value *set_file(Env *env, Value *file) {
        file->assert_type(env, Value::Type::String, "String");
        m_file = file->as_string()->c_str();
        return file;
    }

    size_t line() { return m_line; }
    void set_line(size_t line) { m_line = line; }
    Value *set_line(Env *env, Value *line) {
        line->assert_type(env, Value::Type::Integer, "Integer");
        m_line = line->as_integer()->to_nat_int_t();
        return line;
    }

private:
    SexpValue(Env *env, std::initializer_list<Value *> list)
        : ArrayValue { env, list } {
        m_klass = env->Object()->const_fetch("Parser")->const_fetch("Sexp")->as_class();
    }

    const char *m_file { nullptr };
    size_t m_line { 0 };
    size_t m_column { 0 };
};

}
