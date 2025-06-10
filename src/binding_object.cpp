#include <natalie.hpp>
#include <natalie/binding_object.hpp>

namespace Natalie {

Value BindingObject::source_location() const {
    auto file = StringObject::create(m_env.file());
    auto line = Value::integer(m_env.line());
    return ArrayObject::create({ file, line });
}

}
