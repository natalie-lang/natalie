#include "natalie.hpp"
#include "natalie/fiber_object.hpp"

namespace Natalie {

static void last_regex_match_assignment_hook(Env *env, Object **current, Object *object) {
    if (!object->is_match_data() && !object->is_nil())
        env->raise("TypeError", "wrong argument type {} (expected MatchData)", object->klass()->inspect_str());
    *current = object;
    env->set_match(object);
    if (!object->is_nil()) {
        env->global_set("$`"_s, object->as_match_data()->pre_match(env));
        env->global_set("$'"_s, object->as_match_data()->post_match(env));
        env->global_set("$+"_s, object->as_match_data()->captures(env)->as_array()->compact(env)->as_array()->last());
    }
}

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
        return info->object();
    else
        return false;
}

Value GlobalEnv::global_get(Env *env, SymbolObject *name) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (!name->is_global_name())
        env->raise_name_error(name, "`{}' is not allowed as a global variable name", name->string());

    auto info = m_global_variables.get(name, env);
    if (info)
        return info->object();
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
        if (name->string() == "$~")
            info->set_assignment_hook(last_regex_match_assignment_hook);
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
    return info->object();
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
