#include "natalie/string.hpp"
#include "natalie/object.hpp"
#include "natalie/string_object.hpp"
#include "natalie/value.hpp"

namespace Natalie {

void String::append(const StringObject *str) {
    if (!str) return;
    append(str->c_str());
}

}
