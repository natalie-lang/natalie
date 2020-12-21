require 'natalie/inline'

__inline__ "#include <sys/stat.h>"

class File
  def self.join(*parts)
    parts.join('/')
  end

  def self.exist?(path)
    __inline__ <<-END
        struct stat sb;
        auto path = args[0];
        path->assert_type(env, Value::Type::String, "String");
        if (stat(path->as_string()->c_str(), &sb) == -1)
            return env->false_obj();
        else
            return env->true_obj();
    END
  end
end
