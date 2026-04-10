#include "natalie.hpp"

namespace Natalie {

Value Utf32EncodingObject::encode(Env *env, EncodingObject *orig_encoding, StringObject *str, EncodeOptions options) const {
    auto *delegate = EncodingObject::get(Encoding::UTF_32BE);
    delegate->encode(env, orig_encoding, str, options);
    if (str->bytesize() > 0) {
        auto with_bom = String("\x00\x00\xFE\xFF", 4);
        with_bom.append(str->string());
        str->set_str(std::move(with_bom));
    }
    str->set_encoding(EncodingObject::get(num()));
    return str;
}

}
