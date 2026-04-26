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

ArrayObject *Method::parameters_array(Env *env) const {
    return parameters_to_array(env, m_parameters, m_arity);
}

// When params is nullptr, synthesize from arity the way MRI does for
// attr_reader/attr_writer/fixed-arity C methods.
ArrayObject *parameters_to_array(Env *env, const ParamDescriptor *params, int arity, bool as_proc) {
    auto array = ArrayObject::create();
    if (!params) {
        if (arity < 0) {
            auto rest = ArrayObject::create();
            rest->push("rest"_s);
            array->push(rest);
        } else {
            for (int i = 0; i < arity; ++i) {
                auto pair = ArrayObject::create();
                pair->push(as_proc ? "opt"_s : "req"_s);
                array->push(pair);
            }
        }
        return array;
    }
    for (auto *p = params; p->kind != ParamKind::End; ++p) {
        auto pair = ArrayObject::create();
        SymbolObject *kind_sym = nullptr;
        switch (p->kind) {
        case ParamKind::Req:
            kind_sym = as_proc ? "opt"_s : "req"_s;
            break;
        case ParamKind::Opt:
            kind_sym = "opt"_s;
            break;
        case ParamKind::Rest:
            kind_sym = "rest"_s;
            break;
        case ParamKind::KeyReq:
            kind_sym = "keyreq"_s;
            break;
        case ParamKind::Key:
            kind_sym = "key"_s;
            break;
        case ParamKind::KeyRest:
            kind_sym = "keyrest"_s;
            break;
        case ParamKind::NoKey:
            kind_sym = "nokey"_s;
            break;
        case ParamKind::Block:
            kind_sym = "block"_s;
            break;
        case ParamKind::End:
            continue;
        }
        pair->push(kind_sym);
        if (p->name)
            pair->push(SymbolObject::intern(p->name));
        array->push(pair);
    }
    return array;
}
}
