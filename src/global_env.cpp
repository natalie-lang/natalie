#include "natalie.hpp"
#include "natalie/fiber_object.hpp"

namespace Natalie {

void GlobalEnv::add_file(Env *env, SymbolObject *name) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    m_files.set(name);

    auto loaded_features = global_get(env, "$\""_s);
    loaded_features->as_array()->push(name->to_s(env));
}

bool GlobalEnv::global_defined(Env *env, SymbolObject *name) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (!name->is_global_name())
        env->raise_name_error(name, "`{}' is not allowed as a global variable name", name->string());

    auto info = m_global_variables.get(name, env);
    if (info)
        return info->object(env);
    else
        return false;
}

Value GlobalEnv::global_get(Env *env, SymbolObject *name) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (!name->is_global_name())
        env->raise_name_error(name, "`{}' is not allowed as a global variable name", name->string());

    auto info = m_global_variables.get(name, env);
    if (info)
        return info->object(env);
    else
        return NilObject::the();
}

Value GlobalEnv::global_set(Env *env, SymbolObject *name, Value val, bool readonly) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (!name->is_global_name())
        env->raise_name_error(name, "`{}' is not allowed as a global variable name", name->string());

    auto info = m_global_variables.get(name, env);
    if (info) {
        if (!readonly && info->is_readonly())
            env->raise_name_error(name, "{} is a read only variable", name->string());
        if (readonly)
            assert(info->is_readonly()); // changing a global to read-only is not anticipated.
        info->set_object(env, val.object());
    } else {
        auto info = new GlobalVariableInfo { val.object(), readonly };
        m_global_variables.put(name, info, env);
    }
    return val;
}

Value GlobalEnv::global_alias(Env *env, SymbolObject *new_name, SymbolObject *old_name) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

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
    return info->object(env);
}

ArrayObject *GlobalEnv::global_list(Env *env) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    auto result = new ArrayObject { m_global_variables.size() + 2 };
    for (const auto &[key, _] : m_global_variables) {
        result->push(key);
    }
    // $! and $@ are handled compile time in Natalie
    result->push("$!"_s);
    result->push("$@"_s);
    return result;
}

void GlobalEnv::global_set_read_hook(Env *env, SymbolObject *name, bool readonly, GlobalVariableInfo::read_hook_t read_hook) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    auto info = m_global_variables.get(name, env);
    if (!info) {
        global_set(env, name, NilObject::the(), readonly);
        info = m_global_variables.get(name, env);
    }
    assert(readonly == info->is_readonly());
    info->set_read_hook(read_hook);
}

void GlobalEnv::global_set_write_hook(Env *env, SymbolObject *name, GlobalVariableInfo::write_hook_t write_hook) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    auto info = m_global_variables.get(name, env);
    if (!info)
        env->raise("ScriptError", "Trying to add a write hook to undefined global variable {}", name->inspect_str(env));
    if (info->is_readonly())
        env->raise("ScriptError", "Trying to add a write hook to readonly global variable {}", name->inspect_str(env));
    info->set_write_hook(write_hook);
}

bool GlobalEnv::is_verbose(Env *env) {
    return global_get(env, "$VERBOSE"_s)->is_truthy();
}

void GlobalEnv::visit_children(Visitor &visitor) {
    for (auto pair : m_global_variables) {
        visitor.visit(pair.first);
        visitor.visit(pair.second);
    }
    for (size_t i = 0; i < EncodingCount; i++)
        visitor.visit(m_Encodings[i]);
    for (auto pair : m_files)
        visitor.visit(pair.first);
    visitor.visit(m_Array);
    visitor.visit(m_BasicObject);
    visitor.visit(m_Binding);
    visitor.visit(m_Class);
    visitor.visit(m_Complex);
    visitor.visit(m_Float);
    visitor.visit(m_Hash);
    visitor.visit(m_Integer);
    visitor.visit(m_Module);
    visitor.visit(m_Object);
    visitor.visit(m_Random);
    visitor.visit(m_Rational);
    visitor.visit(m_Regexp);
    visitor.visit(m_String);
    visitor.visit(m_Symbol);
    visitor.visit(m_Time);
    visitor.visit(m_main_obj);
    visitor.visit(m_main_env);
}

}
