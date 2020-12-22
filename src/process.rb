require 'natalie/inline'

__inline__ "#include <sys/wait.h>"

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
  end

  def self.wait(pid = -1, flags = 0)
    __inline__ <<-END
        // TODO: find a better way to get args into inline code...
        Value *pid, *flags;
        if (argc >= 1)
            pid = args[0];
        else
            pid = new IntegerValue { env, -1 };
        if (argc >= 2)
            flags = args[1];
        else
            flags = new IntegerValue { env, 0 };
        pid->assert_type(env, Value::Type::Integer, "Integer");
        flags->assert_type(env, Value::Type::Integer, "Integer");
        int status;
        auto result = waitpid(pid->as_integer()->to_nat_int_t(), &status, flags->as_integer()->to_nat_int_t());
        if (result == -1)
            env->raise_errno();
        set_status_object(env, result, status);
        return new IntegerValue { env, static_cast<nat_int_t>(result) };
    END
  end
end
