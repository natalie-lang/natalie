require 'natalie/inline'

__inline__ '#include <dirent.h>'
__inline__ '#include <sys/param.h>'
__inline__ '#include <sys/types.h>'
__inline__ '#include <unistd.h>'

class Dir
  class << self
    def tmpdir
      '/tmp'
    end

    __define_method__ :pwd, [], <<-END
      char buf[MAXPATHLEN + 1];
      if(!getcwd(buf, MAXPATHLEN + 1))
          env->raise_errno();
      return new StringObject { buf };
    END

    def each_child(dirname)
      return enum_for(:each_child, dirname) unless block_given?
      children(dirname).each { |name| yield name }
    end

    __define_method__ :children, [:dirname], <<-END
      dirname->assert_type(env, Object::Type::String, "String");
      auto dir = opendir(dirname->as_string()->c_str());
      if (!dir)
          env->raise_errno();
      dirent *entry;
      errno = 0;
      auto array = new ArrayObject {};
      for (;;) {
          entry = readdir(dir);
          if (!entry) break;
          if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
          array->push(new StringObject { entry->d_name });
      }
      if (errno) {
          closedir(dir);
          env->raise_errno();
      }
      closedir(dir);
      return array;
    END

    __define_method__ :rmdir, [:dirname], <<-END
      dirname->assert_type(env, Object::Type::String, "String");
      auto result = rmdir(dirname->as_string()->c_str());
      if (result == -1)
          env->raise_errno();
      return Value::integer(result);
    END
  end
end
