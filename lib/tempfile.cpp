#include "natalie.hpp"

using namespace Natalie;

Value init_tempfile(Env *env, Value self) {
    return NilObject::the();
}

Value Tempfile_initialize(Env *env, Value self, Args args, Block *) {
    auto kwargs = args.pop_keyword_hash();
    args.ensure_argc_between(env, 1, 2);
    auto basename = args.at(0);
    auto tmpdir = args.at(1, nullptr);

    basename->assert_type(env, Object::Type::String, "String");
    if (tmpdir && !tmpdir->is_nil()) {
        tmpdir->assert_type(env, Object::Type::String, "String");
    } else {
        tmpdir = GlobalEnv::the()->Object()->const_fetch("Dir"_s).send(env, "tmpdir"_s);
    }
    char path[PATH_MAX + 1];
    auto written = snprintf(path, PATH_MAX + 1, "%s/%sXXXXXX", tmpdir->as_string()->c_str(), basename->as_string()->c_str());
    assert(written < PATH_MAX + 1);
    int fileno = mkstemp(path);
    if (fileno == -1)
        env->raise_errno();

    auto file = new FileObject {};
    file->initialize(env, Args({ Value::integer(fileno), kwargs }, true), nullptr);
    file->set_path(path);

    self->ivar_set(env, "@tmpfile"_s, file);
    return self;
}
