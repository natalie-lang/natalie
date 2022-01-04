#include "natalie.hpp"

namespace Natalie {

Value MethodObject::inspect(Env *env) {
    if (owner()->is_class() && owner()->as_class()->is_singleton()) {
        if (m_method->name() != m_method->original_name()) {
            return StringObject::format(env, "#<Method: {}.{}({})(*)>", m_object->inspect_str(env), m_method->name(), m_method->original_name());
        } else {
            return StringObject::format(env, "#<Method: {}.{}(*)>", m_object->inspect_str(env), m_method->name());
        }
    } else {
        if (m_method->name() != m_method->original_name()) {
            return StringObject::format(env, "#<Method: {}#{}({})(*)>", owner()->inspect_str(), m_method->name(), m_method->original_name());
        } else {
            return StringObject::format(env, "#<Method: {}#{}(*)>", owner()->inspect_str(), m_method->name());
        }
    }
}

}
