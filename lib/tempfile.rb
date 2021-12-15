require 'natalie/inline'

class Tempfile
  class << self
    def new(basename)
      f = Tempfile.create(basename)
      yield f
      f
    end

    __define_method__ :create, [:basename], <<-END
      basename->assert_type(env, Object::Type::String, "String");
      auto tmpdir = GlobalEnv::the()->Object()->const_fetch(SymbolObject::intern("Dir")).send(env, SymbolObject::intern("tmpdir"))->as_string();
      auto path_template = String::format("{}/{}XXXXXX", tmpdir, basename->as_string());
      int fileno = mkstemp(const_cast<char*>(path_template->c_str()));
      if (fileno == -1) {
          Value args[] = { Value::integer(errno) };
          auto exception = GlobalEnv::the()->Object()->const_fetch(SymbolObject::intern("SystemCallError")).send(env, SymbolObject::intern("exception"), 1, args)->as_exception();
          env->raise_exception(exception);
      } else {
          auto file = new FileObject {};
          file->set_fileno(fileno);
          file->set_path(path_template);
          return file;
      }
    END
  end
end
