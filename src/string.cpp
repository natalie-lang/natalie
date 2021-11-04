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

Natalie::String *nat_string(const char *str) {
    return new Natalie::String { str };
}

const char *nat_string_to_c_str(const Natalie::String *string) {
    return string->c_str();
}
