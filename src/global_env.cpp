#include "natalie.hpp"
#include "natalie/fiber_object.hpp"

namespace Natalie {

void GlobalEnv::add_file(Env *env, SymbolObject *name) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    m_files.set(name);

    auto loaded_features = global_get(env, "$\""_s);
    loaded_features.as_array()->push(name->to_s(env));
}

bool GlobalEnv::global_defined(Env *env, SymbolObject *name) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (!name->is_global_name())
        env->raise_name_error(name, "`{}' is not allowed as a global variable name", name->string());

    auto info = m_global_variables.get(name, env);
    if (info) {
        auto obj = info->object(env);
        if (obj)
            return true;
    }

    return false;
}

Value GlobalEnv::global_get(Env *env, SymbolObject *name) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (!name->is_global_name())
        env->raise_name_error(name, "`{}' is not allowed as a global variable name", name->string());

    auto info = m_global_variables.get(name, env);
    if (info) {
        auto obj = info->object(env);
        if (obj)
            return obj.value();
    }

    return Value::nil();
}

Value GlobalEnv::global_set(Env *env, SymbolObject *name, Optional<Value> val, bool readonly) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (!name->is_global_name())
        env->raise_name_error(name, "`{}' is not allowed as a global variable name", name->string());

    auto info = m_global_variables.get(name, env);
    if (info) {
        if (!readonly && info->is_readonly())
            env->raise_name_error(name, "{} is a read-only variable", name->string());
        if (readonly)
            assert(info->is_readonly()); // changing a global to read-only is not anticipated.
        if (val)
            info->set_object(env, val.value());
    } else {
        auto info = new GlobalVariableInfo { name, val, readonly };
        m_global_variables.put(name, info, env);
    }
    return val.value_or(Value::nil());
}

Value GlobalEnv::global_alias(Env *env, SymbolObject *new_name, SymbolObject *old_name) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (!new_name->is_global_name())
        env->raise_name_error(new_name, "`{}' is not allowed as a global variable name", new_name->string());
    if (!old_name->is_global_name())
        env->raise_name_error(old_name, "`{}' is not allowed as a global variable name", old_name->string());

    auto info = m_global_variables.get(old_name, env);
    if (!info) {
        global_set(env, old_name, Value::nil());
        info = m_global_variables.get(old_name, env);
    }
    m_global_variables.put(new_name, info, env);
    return info->object(env).value_or(Value::nil());
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
        global_set(env, name, {}, readonly);
        info = m_global_variables.get(name, env);
    }
    assert(readonly == info->is_readonly());
    info->set_read_hook(read_hook);
}

void GlobalEnv::global_set_write_hook(Env *env, SymbolObject *name, GlobalVariableInfo::write_hook_t write_hook) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    auto info = m_global_variables.get(name, env);
    if (!info)
        env->raise("ScriptError", "Trying to add a write hook to undefined global variable {}", name->inspected(env));
    if (info->is_readonly())
        env->raise("ScriptError", "Trying to add a write hook to readonly global variable {}", name->inspected(env));
    info->set_write_hook(write_hook);
}

bool GlobalEnv::show_deprecation_warnings(Env *env) {
    return Object()->const_fetch("Warning"_s).send(env, "[]"_s, { "deprecated"_s }).is_truthy();
}

void GlobalEnv::set_interned_strings(StringObject **interned_strings, const size_t interned_strings_size) {
    m_interned_strings.push({ interned_strings, interned_strings_size });
}

void GlobalEnv::visit_children(Visitor &visitor) const {
    for (auto pair : m_global_variables) {
        visitor.visit(pair.first);
        visitor.visit(pair.second);
    }
    for (size_t i = 0; i < EncodingCount; i++)
        visitor.visit(m_Encodings[i]);
    for (auto pair : m_files)
        visitor.visit(pair.first);
    for (const auto &span : m_interned_strings)
        for (auto str : span)
            visitor.visit(str);
    for (const auto &instance_eval_context : m_instance_eval_contexts) {
        visitor.visit(instance_eval_context.caller_env);
        visitor.visit(instance_eval_context.block_original_self);
    }
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
