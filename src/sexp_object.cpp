#include "natalie/sexp_object.hpp"
#include "natalie.hpp"
#include "natalie/node.hpp"

namespace Natalie {

SexpObject::SexpObject(Env *env, Node *node, std::initializer_list<Value> list)
    : ArrayObject { list } {
    m_klass = GlobalEnv::the()->Object()->const_fetch(SymbolObject::intern("Sexp"))->as_class();
    if (node->file())
        ivar_set(env, SymbolObject::intern("@file"), new StringObject { node->file() });
    ivar_set(env, SymbolObject::intern("@line"), Value::integer(static_cast<nat_int_t>(node->line())));
    ivar_set(env, SymbolObject::intern("@column"), Value::integer(static_cast<nat_int_t>(node->column())));
}

}
