#include "natalie.hpp"

using namespace Natalie;

Value init_tempfile(Env *env, Value self) {
    return NilObject::the();
}

Value Tempfile_initialize(Env *env, Value self, Args args, Block *) {
    auto kwargs = args.pop_keyword_hash();
    args.ensure_argc_between(env, 0, 2);
    auto basename = args.at(0, nullptr);
    auto tmpdir = args.at(1, nullptr);
    auto suffix = new StringObject { "" };

    if (!basename) {
        basename = new StringObject { "" };
    } else if (!basename->is_string() && basename->respond_to(env, "to_str"_s)) {
        basename = basename->to_str(env);
    } else if (basename->is_array()) {
        auto arr = basename->as_array();
        basename = arr->ref(env, Value::integer(0));
        if (arr->size() >= 2)
            suffix = arr->at(1)->to_str(env);
    }
    if (!basename->is_string()) {
        env->raise("ArgumentError", "unexpected prefix: {}", basename->inspect_str(env));
    }
    if (tmpdir && !tmpdir->is_nil()) {
        tmpdir->assert_type(env, Object::Type::String, "String");
    } else {
        tmpdir = GlobalEnv::the()->Object()->const_fetch("Dir"_s).send(env, "tmpdir"_s);
    }
    char path[PATH_MAX + 1];
    auto written = snprintf(path, PATH_MAX + 1, "%s/%sXXXXXX%s", tmpdir->as_string()->c_str(), basename->as_string()->c_str(), suffix->c_str());
    assert(written < PATH_MAX + 1);
    int fileno = mkstemps(path, suffix->bytesize());
    if (fileno == -1)
        env->raise_errno();

    auto file = new FileObject {};
    file->initialize(env, Args({ Value::integer(fileno), kwargs }, true), nullptr);
    file->set_path(path);

    self->ivar_set(env, "@tmpfile"_s, file);
    return self;
}
