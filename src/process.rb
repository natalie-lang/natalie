require 'natalie/inline'

__inline__ '#include <sys/wait.h>'

module Process
  __constant__('PRIO_PGRP', 'int')
  __constant__('PRIO_PROCESS', 'int')
  __constant__('PRIO_USER', 'int')
  __constant__('RLIMIT_AS', 'int')
  __constant__('RLIMIT_CORE', 'int')
  __constant__('RLIMIT_CPU', 'int')
  __constant__('RLIMIT_DATA', 'int')
  __constant__('RLIMIT_FSIZE', 'int')
  __constant__('RLIMIT_MEMLOCK', 'int')
  __constant__('RLIMIT_MSGQUEUE', 'int')
  __constant__('RLIMIT_NICE', 'int')
  __constant__('RLIMIT_NOFILE', 'int')
  __constant__('RLIMIT_NPROC', 'int')
  __constant__('RLIMIT_NPTS', 'int')
  __constant__('RLIMIT_RSS', 'int')
  __constant__('RLIMIT_RTPRIO', 'int')
  __constant__('RLIMIT_RTTIME', 'int')
  __constant__('RLIMIT_SBSIZE', 'int')
  __constant__('RLIMIT_SIGPENDING', 'int')
  __constant__('RLIMIT_STACK', 'int')
  __constant__('RLIM_INFINITY', 'unsigned long long')
  __constant__('RLIM_SAVED_CUR', 'unsigned long long')
  __constant__('RLIM_SAVED_MAX', 'unsigned long long')
  __constant__('WNOHANG', 'int')
  __constant__('WUNTRACED', 'int')

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
        args.ensure_argc_between(env, 0, 2);
        nat_int_t pid = -1, flags = 0;
        arg_spread(env, args, "|ii", &pid, &flags);
        int status;
        auto result = waitpid(pid, &status, flags);
        if (result == -1)
            env->raise_errno();
        set_status_object(env, result, status);
        return Value::integer(static_cast<nat_int_t>(result));
    END
  end
end
