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
      auto tmpdir = env->Object()->const_fetch(SymbolValue::intern("Dir")).send(env, "tmpdir")->as_string();
      auto path_template = String::format("{}/{}XXXXXX", tmpdir, basename->as_string());
      int fileno = mkstemp(const_cast<char*>(path_template->c_str()));
      if (fileno == -1) {
          ValuePtr args[] = { ValuePtr::integer(errno) };
          auto exception = env->Object()->const_fetch(SymbolValue::intern("SystemCallError")).send(env, "exception", 1, args)->as_exception();
          env->raise_exception(exception);
      } else {
          auto file = new FileValue {};
          file->set_fileno(fileno);
          file->set_path(path_template);
          return file;
      }
    END
  end
end
