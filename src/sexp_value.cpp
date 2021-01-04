#include "natalie/sexp_value.hpp"
#include "natalie.hpp"
#include "natalie/node.hpp"

namespace Natalie {

SexpValue::SexpValue(Env *env, Node *node, std::initializer_list<Value *> list)
    : ArrayValue { env, list }
    , m_file { node->file() }
    , m_line { node->line() }
    , m_column { node->column() } {
    m_klass = env->Object()->const_fetch("Parser")->const_fetch("Sexp")->as_class();
}

}
