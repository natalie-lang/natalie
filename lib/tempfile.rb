require 'natalie/inline'

class Tempfile
  class << self
    def create(basename)
      if block_given?
        begin
          tmpfile = new(basename)
          yield(tmpfile.instance_variable_get(:@tmpfile))
        ensure
          tmpfile.unlink
        end
      else
        new(basename).instance_variable_get(:@tmpfile)
      end
    end
  end

  __define_method__ :initialize, [:basename], <<~CPP
    basename->assert_type(env, Object::Type::String, "String");
    auto tmpdir = GlobalEnv::the()->Object()->const_fetch("Dir"_s).send(env, "tmpdir"_s)->as_string();
    char path[PATH_MAX+1];
    auto written = snprintf(path, PATH_MAX+1, "%s/%sXXXXXX", tmpdir->c_str(), basename->as_string()->c_str());
    assert(written < PATH_MAX+1);
    int fileno = mkstemp(path);
    if (fileno == -1)
        env->raise_errno();

    auto file = new FileObject {};
    file->set_fileno(fileno);
    file->set_path(path);

    self->ivar_set(env, "@tmpfile"_s, file);
    return self;
  CPP

  def unlink
    File.unlink(@tmpfile.path)
  end
  alias delete unlink

  (File.public_instance_methods(false) + [:close, :open, :path, :print, :puts, :rewind, :write]).each do |method|
    define_method(method) do |*args, **kwargs, &block|
      @tmpfile.public_send(method, *args, **kwargs, &block)
    end
  end
end
