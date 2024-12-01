class Thread
  class ConditionVariable
    def initialize
      @mutex = Mutex.new
      @waiting = []
    end

    def broadcast
      @mutex.synchronize do
        until @waiting.empty?
          thread = @waiting.shift
          if thread.status != 'dead'
            thread.wakeup
          end
        end
      end
    end

    def marshal_dump
      raise TypeError, "can't dump #{self.class}"
    end

    def signal
      @mutex.synchronize do
        thread = nil
        until @waiting.empty?
          thread = @waiting.shift
          if thread.status != 'dead'
            thread.wakeup
            break
          end
        end
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
