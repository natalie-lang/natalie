class Thread
  class ConditionVariable
    def initialize
      @mutex = Mutex.new
      @waiting = []
    end

    def broadcast
      @mutex.synchronize do
        while !@waiting.empty?
          thread = @waiting.shift
          thread.wakeup if thread&.status == 'sleep'
        end
      end
    end

    def marshal_dump
      raise TypeError, "can't dump #{self.class}"
    end

    def signal
      @mutex.synchronize do
        thread = nil
        while !@waiting.empty? && thread&.status != 'sleep'
          thread = @waiting.shift
        end
        thread&.wakeup
      end
    end

    def wait(mutex, timeout = nil)
      @mutex.synchronize do
        @waiting << Thread.current
      end
      mutex.sleep(timeout)
    end
  end
end

ConditionVariable = Thread::ConditionVariable
