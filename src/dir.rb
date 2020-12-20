require 'natalie/inline'

__inline__ "#include <sys/types.h>"
__inline__ "#include <dirent.h>"

class Dir
  def self.tmpdir
    '/tmp'
  end

  def self.children(dirname)
    each_child(dirname).to_a
  end

  def self.each_child(dirname)
    __inline__ <<-END
        auto dirname = args[0];
        dirname->assert_type(env, Value::Type::String, "String");
        if (!block) {
            Value *enum_for_args[] = { SymbolValue::intern(env, "each_child"), args[0] };
            return self->send(env, "enum_for", 2, enum_for_args, nullptr);
        }
        auto dir = opendir(dirname->as_string()->c_str());
        if (!dir) {
            Value *args[] = { new IntegerValue { env, errno } };
            auto exception = env->Object()->const_fetch("SystemCallError")->send(env, "exception", 1, args)->as_exception();
            env->raise_exception(exception);
        }
        errno = 0;
        dirent *entry;
        for (;;) {
            entry = readdir(dir);
            if (!entry) break;
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            Value *args[] = { new StringValue { env, entry->d_name } };
            NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, args, nullptr);
        }
        if (errno) {
            closedir(dir);
            Value *args[] = { new IntegerValue { env, errno } };
            auto exception = env->Object()->const_fetch("SystemCallError")->send(env, "exception", 1, args)->as_exception();
            env->raise_exception(exception);
        }
        closedir(dir);
        return env->nil_obj();
    END
  end
end
