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

void GlobalEnv::visit_children(Visitor &visitor) {
    for (auto pair : m_globals) {
        visitor.visit(pair.first);
        visitor.visit(pair.second);
    }
    for (auto pair : m_symbols) {
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
    visitor.visit(m_nil_obj);
    visitor.visit(m_true_obj);
    visitor.visit(m_false_obj);
    visitor.visit(m_main_fiber);
    visitor.visit(m_current_fiber);
    for (size_t i = 0; i < m_fiber_args.argc; ++i) {
        visitor.visit(m_fiber_args.args[i]);
    }
}

}
