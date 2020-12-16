require 'natalie/inline'

class Tempfile
  def self.create(basename)
    __inline__ <<-END
      auto tmpdir = env->Object()->const_fetch("Dir")->send(env, "tmpdir")->as_string();
      auto path_template = StringValue::sprintf(env, "%S/%SXXXXXX", tmpdir, args[0]->as_string());
      auto generated_path = GC_STRDUP(path_template->c_str());
      int fileno = mkstemp(generated_path);
      if (fileno == -1) {
          Value *args[] = { new IntegerValue { env, errno } };
          auto exception = env->Object()->const_fetch("SystemCallError")->send(env, "exception", 1, args)->as_exception();
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
