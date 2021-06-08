#include "natalie.hpp"
#include "natalie/fiber_value.hpp"

namespace Natalie {

FiberValue *GlobalEnv::main_fiber(Env *env) {
    if (m_main_fiber) return m_main_fiber;
    m_main_fiber = new FiberValue { Heap::the().start_of_stack() };
    return m_main_fiber;
}

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

void GlobalEnv::set_fiber_args(size_t argc, ValuePtr *args) {
    m_fiber_args.clear();
    for (size_t i = 0; i < argc; ++i) {
        m_fiber_args.push(args[i]);
    }
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
    visitor.visit(m_main_fiber);
    visitor.visit(m_current_fiber);
    for (auto arg : m_fiber_args) {
        visitor.visit(arg);
    }
}

}
