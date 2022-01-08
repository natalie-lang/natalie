require 'natalie/inline'

__inline__ '#include <sys/wait.h>'

module Process
  class Status
    attr_accessor :pid
    attr_accessor :to_i
    attr_accessor :exitstatus

    def inspect
      "#<Process::Status: pid #{pid} exit #{exitstatus}>"
    end

    def ==(other)
      other.is_a?(Integer) && to_i == other
    end

    def success?
      exitstatus == 0
    end
  end

  class << self
    __define_method__ :wait, <<-END
        env->ensure_argc_between(argc, 0, 2);
        nat_int_t pid = -1, flags = 0;
        arg_spread(env, argc, args, "|ii", &pid, &flags);
        int status;
        auto result = waitpid(pid, &status, flags);
        if (result == -1)
            env->raise_errno();
        set_status_object(env, result, status);
        return Value::integer(static_cast<nat_int_t>(result));
    END
  end
end
