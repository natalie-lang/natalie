require 'natalie/inline'

class Tempfile
  class << self
    def new(basename)
      Tempfile.create(basename)
    end

    __define_method__ :create, [:basename], <<-CPP
      basename->assert_type(env, Object::Type::String, "String");
      auto tmpdir = GlobalEnv::the()->Object()->const_fetch("Dir"_s).send(env, "tmpdir"_s)->as_string();
      char path[PATH_MAX+1];
      auto written = snprintf(path, PATH_MAX+1, "%s/%sXXXXXX", tmpdir->c_str(), basename->as_string()->c_str());
      assert(written < PATH_MAX+1);
      int fileno = mkstemp(path);
      if (fileno == -1) {
          Value args[] = { Value::integer(errno) };
          auto exception = GlobalEnv::the()->Object()->const_fetch("SystemCallError"_s).send(env, "exception"_s, Args(1, args))->as_exception();
          env->raise_exception(exception);
      } else {
          auto file = new FileObject {};
          file->set_fileno(fileno);
          file->set_path(path);
          if(block) {
            Defer close_file([&]() {
              file->close(env);
              FileObject::unlink(env, new StringObject { path });
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
