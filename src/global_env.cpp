#include "natalie.hpp"
#include "natalie/fiber_object.hpp"

namespace Natalie {

std::mutex g_gvar_mutex;
std::mutex g_gvar_alias_mutex;

bool GlobalEnv::global_defined(Env *env, SymbolObject *name) {
    std::lock_guard<std::mutex> lock(g_gvar_mutex);

    if (!name->is_global_name())
        env->raise_name_error(name, "`{}' is not allowed as a global variable name", name->string());

    auto info = m_global_variables.get(name, env);
    if (info)
        return info->object();
    else
        return false;
}

Value GlobalEnv::global_get(Env *env, SymbolObject *name) {
    std::lock_guard<std::mutex> lock(g_gvar_mutex);

    if (!name->is_global_name())
        env->raise_name_error(name, "`{}' is not allowed as a global variable name", name->string());

    auto info = m_global_variables.get(name, env);
    if (info)
        return info->object();
    else
        return NilObject::the();
}

Value GlobalEnv::global_set(Env *env, SymbolObject *name, Value val) {
    std::lock_guard<std::mutex> lock(g_gvar_mutex);

    if (!name->is_global_name())
        env->raise_name_error(name, "`{}' is not allowed as a global variable name", name->string());

    auto info = m_global_variables.get(name, env);
    if (info) {
        info->set_object(val.object());
    } else {
        auto info = new GlobalVariableInfo { val.object() };
        m_global_variables.put(name, info, env);
    }
    return val;
}

Value GlobalEnv::global_alias(Env *env, SymbolObject *new_name, SymbolObject *old_name) {
    std::lock_guard<std::mutex> lock(g_gvar_alias_mutex);

    if (!new_name->is_global_name())
        env->raise_name_error(new_name, "`{}' is not allowed as a global variable name", new_name->string());
    if (!old_name->is_global_name())
        env->raise_name_error(old_name, "`{}' is not allowed as a global variable name", old_name->string());

    auto info = m_global_variables.get(old_name, env);
    if (!info) {
        global_set(env, old_name, NilObject::the());
        info = m_global_variables.get(old_name, env);
    }
    m_global_variables.put(new_name, info, env);
    return info->object();
}

}
