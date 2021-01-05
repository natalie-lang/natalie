require 'natalie/inline'

__inline__ "#include <dirent.h>"
__inline__ "#include <sys/param.h>"
__inline__ "#include <sys/types.h>"

class Dir
  def self.tmpdir
    '/tmp'
  end

  def self.pwd
    __inline__ <<-END
        char buf[MAXPATHLEN + 1];
        if(!getcwd(buf, MAXPATHLEN + 1))
            env->raise_errno();
        return new StringValue { env, buf };
    END
  end

  def self.each_child(dirname)
    return enum_for(:each_child, dirname) unless block_given?
    children(dirname).each do |name|
      yield name
    end
  end

  def self.children(dirname)
    __inline__ <<-END
        auto dirname = args[0];
        dirname->assert_type(env, Value::Type::String, "String");
        auto dir = opendir(dirname->as_string()->c_str());
        if (!dir)
            env->raise_errno();
        dirent *entry;
        errno = 0;
        auto array = new ArrayValue { env };
        for (;;) {
            entry = readdir(dir);
            if (!entry) break;
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            array->push(new StringValue { env, entry->d_name });
        }
        if (errno) {
            closedir(dir);
            env->raise_errno();
        }
        closedir(dir);
        return array;
    END
  end
end
