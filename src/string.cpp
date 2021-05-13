#include "natalie/string.hpp"
#include "natalie/string_value.hpp"
#include "natalie/value.hpp"
#include "natalie/value_ptr.hpp"

namespace Natalie {

void String::append(const StringValue *str) {
    if (!str) return;
    append(str->c_str());
}

}
