#include "natalie.hpp"

namespace Natalie {

EncodingValue *encoding(Env *env, Encoding num, ArrayValue *names) {
    EncodingValue *encoding = new EncodingValue { env };
    encoding->encoding_num = num;
    encoding->encoding_names = names;
    freeze_object(names);
    return encoding;
}

}
