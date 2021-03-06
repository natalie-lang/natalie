#include "natalie.hpp"
#include "natalie/fiber_value.hpp"

namespace Natalie {

FiberValue *GlobalEnv::main_fiber(Env *env) {
    if (m_main_fiber) return m_main_fiber;
    m_main_fiber = new FiberValue { env };
    m_main_fiber->set_current_stack_bottom();
    return m_main_fiber;
}

ValuePtr GlobalEnv::global_get(Env *env, SymbolValue *name) {
    if (!name->is_global_name())
        env->raise("NameError", "`{}' is not allowed as an global variable name", name->c_str());

    auto val = m_globals.get(env, name);
    if (val)
        return val;
    else
        return nil_obj();
}

ValuePtr GlobalEnv::global_set(Env *env, SymbolValue *name, ValuePtr val) {
    if (!name->is_global_name())
        env->raise("NameError", "`{}' is not allowed as an global variable name", name->c_str());

    m_globals.put(env, name, val.value());
    return val;
}

SymbolValue *GlobalEnv::symbol_get(Env *env, const char *name) {
    return m_symbols.get(env, name);
}

void GlobalEnv::symbol_set(Env *env, const char *name, SymbolValue *val) {
    m_symbols.put(env, name, val);
}

}
