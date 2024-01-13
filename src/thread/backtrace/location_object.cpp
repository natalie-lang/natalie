#include "natalie.hpp"

namespace Natalie::Thread::Backtrace {

Value LocationObject::absolute_path(Env *env) const {
    return FileObject::expand_path(env, m_file, DirObject::pwd(env));
}

StringObject *LocationObject::inspect(Env *env) const {
    return to_s()->inspect(env);
}

StringObject *LocationObject::to_s() const {
    return StringObject::format("{}:{}:in `{}'", m_file, m_line, m_source_location);
}

}
