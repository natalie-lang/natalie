#include "natalie.hpp"
#include "natalie/fiber_object.hpp"

namespace Natalie {

bool GlobalEnv::global_defined(Env *env, SymbolObject *name) {
    if (!name->is_global_name())
        env->raise_name_error(name, "`{}' is not allowed as a global variable name", name->string());

    auto info = m_global_variables.get(name, env);
    if (info)
        return info->object();
    else
        return false;
}

Value GlobalEnv::global_get(Env *env, SymbolObject *name) {
    if (!name->is_global_name())
        env->raise_name_error(name, "`{}' is not allowed as a global variable name", name->string());

    auto info = m_global_variables.get(name, env);
    if (info)
        return info->object();
    else
        return NilObject::the();
}

Value GlobalEnv::global_set(Env *env, SymbolObject *name, Value val) {
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
    if (!new_name->is_global_name())
        env->raise_name_error(new_name, "`{}' is not allowed as a global variable name", new_name->string());
    if (!old_name->is_global_name())
        env->raise_name_error(old_name, "`{}' is not allowed as a global variable name", old_name->string());

    auto info = m_global_variables.get(old_name, env);
    if (info)
        m_global_variables.put(new_name, info, env);
    return info->object();
}

void GlobalEnv::visit_children(Visitor &visitor) {
    for (auto pair : m_global_variables) {
        visitor.visit(pair.first);
        visitor.visit(pair.second);
    }
    for (size_t i = 0; i < EncodingCount; i++)
        visitor.visit(m_Encodings[i]);
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
