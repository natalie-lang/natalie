#include "natalie/string.hpp"
#include "natalie/string_value.hpp"
#include "natalie/value.hpp"
#include "natalie/value_ptr.hpp"

namespace Natalie {

void String::append(const StringValue *string2) {
    append(string2->c_str());
}

}
