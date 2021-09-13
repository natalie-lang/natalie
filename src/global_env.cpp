#include "natalie.hpp"
#include "natalie/fiber_value.hpp"

namespace Natalie {

ValuePtr GlobalEnv::global_get(Env *env, SymbolValue *name) {
    if (!name->is_global_name())
        env->raise("NameError", "`{}' is not allowed as an global variable name", name->c_str());

    auto val = m_globals.get(name, env);
    if (val)
        return val;
    else
        return NilValue::the();
}

ValuePtr GlobalEnv::global_set(Env *env, SymbolValue *name, ValuePtr val) {
    if (!name->is_global_name())
        env->raise("NameError", "`{}' is not allowed as an global variable name", name->c_str());

    m_globals.put(name, val.value(), env);
    return val;
}

void GlobalEnv::visit_children(Visitor &visitor) {
    for (auto pair : m_globals) {
        visitor.visit(pair.first);
        visitor.visit(pair.second);
    }
    visitor.visit(m_Array);
    visitor.visit(m_Class);
    visitor.visit(m_Hash);
    visitor.visit(m_Integer);
    visitor.visit(m_Module);
    visitor.visit(m_Object);
    visitor.visit(m_Regexp);
    visitor.visit(m_String);
    visitor.visit(m_Symbol);
    visitor.visit(m_main_obj);
}

}
