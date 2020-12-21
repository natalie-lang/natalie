module Process
  class Status
    attr_accessor :pid
    attr_accessor :exitstatus

    def inspect
      "#<Process::Status: pid #{pid} exit #{exitstatus}>"
    end
  end
end
