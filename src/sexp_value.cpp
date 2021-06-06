#include "natalie/sexp_value.hpp"
#include "natalie.hpp"
#include "natalie/node.hpp"

namespace Natalie {

SexpValue::SexpValue(Env *env, Node *node, std::initializer_list<ValuePtr> list)
    : ArrayValue { list } {
    m_klass = env->Object()->const_fetch(SymbolValue::intern("Parser"))->const_fetch(SymbolValue::intern("Sexp"))->as_class();
    if (node->file())
        ivar_set(env, SymbolValue::intern("@file"), new StringValue { env, node->file() });
    ivar_set(env, SymbolValue::intern("@line"), ValuePtr::integer(static_cast<nat_int_t>(node->line())));
    ivar_set(env, SymbolValue::intern("@column"), ValuePtr::integer(static_cast<nat_int_t>(node->column())));
}

}
