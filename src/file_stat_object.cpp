#include "natalie.hpp"
#include <filesystem>

namespace Natalie {

Value FileStatObject::initialize(Env *env, Value path) {
    if (!path->is_string() && path->respond_to(env, "to_path"_s))
        path = path->send(env, "to_path"_s, { path });

    path->assert_type(env, Object::Type::String, "String");

    std::error_code ec;
    fstatus = std::filesystem::status(path->as_string()->c_str(), ec);
    if (ec) {
        errno = ec.value();
        env->raise_errno();
    }
    return this;
}

// NATFIXME: Support other file-types
Value FileStatObject::ftype() {
    std::filesystem::file_type ftype = fstatus.type();
    if (ftype == std::filesystem::file_type::regular) {
        return new StringObject { "file" };
    }
    return NilObject::the();
}
}
