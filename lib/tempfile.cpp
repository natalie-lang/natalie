#include "natalie.hpp"

using namespace Natalie;

Value init_tempfile(Env *env, Value self) {
    return NilObject::the();
}

Value Tempfile_initialize(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    auto basename = args.at(0);

    basename->assert_type(env, Object::Type::String, "String");
    auto tmpdir = GlobalEnv::the()->Object()->const_fetch("Dir"_s).send(env, "tmpdir"_s)->as_string();
    char path[PATH_MAX + 1];
    auto written = snprintf(path, PATH_MAX + 1, "%s/%sXXXXXX", tmpdir->c_str(), basename->as_string()->c_str());
    assert(written < PATH_MAX + 1);
    int fileno = mkstemp(path);
    if (fileno == -1)
        env->raise_errno();

    auto file = new FileObject {};
    file->set_fileno(fileno);
    file->set_path(path);

    self->ivar_set(env, "@tmpfile"_s, file);
    return self;
}
