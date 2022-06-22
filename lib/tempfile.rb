require 'natalie/inline'

class Tempfile
  class << self
    def new(basename)
      Tempfile.create(basename)
    end

    __define_method__ :create, [:basename], <<-CPP
      basename->assert_type(env, Object::Type::String, "String");
      auto tmpdir = GlobalEnv::the()->Object()->const_fetch("Dir"_s).send(env, "tmpdir"_s)->as_string();
      auto path_template = ManagedString::format("{}/{}XXXXXX", tmpdir, basename->as_string());
      int fileno = mkstemp(const_cast<char*>(path_template->c_str()));
      if (fileno == -1) {
          Value args[] = { Value::integer(errno) };
          auto exception = GlobalEnv::the()->Object()->const_fetch("SystemCallError"_s).send(env, "exception"_s, Args(1, args))->as_exception();
          env->raise_exception(exception);
      } else {
          auto file = new FileObject {};
          file->set_fileno(fileno);
          file->set_path(path_template);
          if(block) {
            Defer close_file([&]() {
              file->close(env);
              FileObject::unlink(env, new StringObject { path_template });
            });
            Value block_args[] = { file };
            return NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args(1, block_args), nullptr);
          } else {
            return file;
          }
      }
    CPP
  end
end
