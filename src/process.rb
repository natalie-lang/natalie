require 'natalie/inline'

__inline__ '#include <sys/wait.h>'
__inline__ '#include <time.h>'

module Process
  __constant__('CLOCK_BOOTTIME', 'int')
  __constant__('CLOCK_BOOTTIME_ALARM', 'int')
  __constant__('CLOCK_MONOTONIC', 'int')
  __constant__('CLOCK_MONOTONIC_COARSE', 'int')
  __constant__('CLOCK_MONOTONIC_FAST', 'int')
  __constant__('CLOCK_MONOTONIC_PRECISE', 'int')
  __constant__('CLOCK_MONOTONIC_RAW', 'int')
  __constant__('CLOCK_MONOTONIC_RAW_APPROX', 'int')
  __constant__('CLOCK_PROCESS_CPUTIME_ID', 'int')
  __constant__('CLOCK_PROF', 'int')
  __constant__('CLOCK_REALTIME', 'int')
  __constant__('CLOCK_REALTIME_ALARM', 'int')
  __constant__('CLOCK_REALTIME_COARSE', 'int')
  __constant__('CLOCK_REALTIME_FAST', 'int')
  __constant__('CLOCK_REALTIME_PRECISE', 'int')
  __constant__('CLOCK_SECOND', 'int')
  __constant__('CLOCK_TAI', 'int')
  __constant__('CLOCK_THREAD_CPUTIME_ID', 'int')
  __constant__('CLOCK_UPTIME', 'int')
  __constant__('CLOCK_UPTIME_FAST', 'int')
  __constant__('CLOCK_UPTIME_PRECISE', 'int')
  __constant__('CLOCK_UPTIME_RAW', 'int')
  __constant__('CLOCK_UPTIME_RAW_APPROX', 'int')
  __constant__('CLOCK_VIRTUAL', 'int')
  __constant__('CLOCK_VIRTUAL', 'int')
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
  __constant__('RLIM_INFINITY', 'bigint')
  __constant__('RLIM_SAVED_CUR', 'bigint')
  __constant__('RLIM_SAVED_MAX', 'bigint')
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

    def exited?
      !signaled?
    end

    def signaled?
      exitstatus & 128 == 128
    end

    def success?
      return nil if signaled?
      exitstatus == 0
    end

    def termsig
      return nil unless signaled?
      exitstatus & 127
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
