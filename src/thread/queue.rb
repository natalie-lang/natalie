class Thread
  class Queue
    def initialize(enum = nil)
      @closed = false
      @mutex = Mutex.new
      if enum.nil?
        @queue = []
      elsif enum.is_a?(Array)
        @queue = enum.dup
      else
        raise TypeError, "can't convert #{enum.class} into Array" unless enum.respond_to?(:to_a)

        @queue = enum.to_a
        raise TypeError, "can't convert #{enum.class} to Array (#{enum.class}#to_a gives #{@queue.class})" unless @queue.is_a?(Array)
      end
      @waiting = []
    end

    def clear
      @mutex.synchronize { @queue.clear }
      self
    end

    def close
      @mutex.synchronize { @closed = true }
      until @waiting.empty?
        begin
          @waiting.pop&.wakeup
        rescue ThreadError
          nil # Thread has woken up by other means, just ignore it
        end
      end
      self
    end

    def closed?
      @mutex.synchronize { @closed }
    end

    def empty?
      @mutex.synchronize { @queue.empty? }
    end

    def freeze
      raise TypeError, "cannot freeze #{self}"
    end

    def length
      @mutex.synchronize { @queue.length }
    end
    alias size length

    def num_waiting
      @mutex.synchronize { @waiting.length }
    end

    def pop(non_block = false, timeout: nil)
      raise NotImplementedError, 'Queue#pop with timeout keyword is not supported' unless timeout.nil?

      @mutex.synchronize do
        raise ThreadError, 'queue empty' if non_block && @queue.empty?
        return @queue.shift if !@queue.empty? || @closed

        @waiting << Thread.current
        while @queue.empty? && !@closed
          @mutex.sleep
        end
        @queue.shift
      ensure
        @waiting.delete(Thread.current)
      end
    end
    alias deq pop
    alias shift pop

    def push(obj)
      thread = nil
      @mutex.synchronize do
        raise ClosedQueueError, 'queue closed' if @closed

        @queue.push(obj)
        thread = @waiting.pop
        while !thread.nil? && !thread.status == 'sleep'
          thread = @waiting.pop
        end
      end
      thread.wakeup if thread&.status == 'sleep'
      self
    end
    alias << push
    alias enq push
  end
end

Queue = Thread::Queue
