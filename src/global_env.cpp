#include "natalie.hpp"
#include "natalie/fiber_object.hpp"

namespace Natalie {

Value GlobalEnv::global_get(Env *env, SymbolObject *name) {
    if (!name->is_global_name())
        env->raise_name_error(name, "`{}' is not allowed as an global variable name", name->c_str());

    auto val = m_globals.get(name, env);
    if (val)
        return val;
    else
        return NilObject::the();
}

Value GlobalEnv::global_set(Env *env, SymbolObject *name, Value val) {
    if (!name->is_global_name())
        env->raise_name_error(name, "`{}' is not allowed as an global variable name", name->c_str());

    m_globals.put(name, val.object(), env);
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
    visitor.visit(m_Random);
    visitor.visit(m_Regexp);
    visitor.visit(m_String);
    visitor.visit(m_Symbol);
    visitor.visit(m_main_obj);
}

}
