#include <natalie.hpp>
#include <natalie/binding_object.hpp>

namespace Natalie {

Value BindingObject::source_location() const {
    auto file = new StringObject { m_env.file() };
    auto line = Value::integer(m_env.line());
    return new ArrayObject { file, line };
}

}
