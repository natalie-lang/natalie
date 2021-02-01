require 'natalie/inline'

class Tempfile
  class << self
    def new(basename)
      f = Tempfile.create(basename)
      yield f
      f
    end

    __define_method__ :create, [:basename], <<-END
      basename->assert_type(env, Value::Type::String, "String");
      auto tmpdir = env->Object()->const_fetch(env, SymbolValue::intern(env, "Dir"))->send(env, "tmpdir")->as_string();
      auto path_template = StringValue::sprintf(env, "%S/%SXXXXXX", tmpdir, basename->as_string());
      auto generated_path = GC_STRDUP(path_template->c_str());
      int fileno = mkstemp(generated_path);
      if (fileno == -1) {
          Value *args[] = { new IntegerValue { env, errno } };
          auto exception = env->Object()->const_fetch(env, SymbolValue::intern(env, "SystemCallError"))->send(env, "exception", 1, args)->as_exception();
          env->raise_exception(exception);
      } else {
          auto file = new FileValue { env };
          file->set_fileno(fileno);
          file->set_path(generated_path);
          return file;
      }
    END
  end
end
