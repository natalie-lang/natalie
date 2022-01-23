#include "natalie/sexp_object.hpp"
#include "natalie.hpp"
#include "natalie/node.hpp"

namespace Natalie {

SexpObject::SexpObject(Env *env, Node *node, std::initializer_list<Value> list)
    : ArrayObject { list } {
    m_klass = GlobalEnv::the()->Object()->const_fetch("Sexp"_s)->as_class();
    if (node->file())
        ivar_set(env, "@file"_s, new StringObject { *node->file() });
    ivar_set(env, "@line"_s, Value::integer(static_cast<nat_int_t>(node->line())));
    ivar_set(env, "@column"_s, Value::integer(static_cast<nat_int_t>(node->column())));
}

}
