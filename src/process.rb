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
end
