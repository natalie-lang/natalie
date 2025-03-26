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
        unless @queue.is_a?(Array)
          raise TypeError, "can't convert #{enum.class} to Array (#{enum.class}#to_a gives #{@queue.class})"
        end
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
      unless timeout.nil?
        raise ArgumentError, "can't set a timeout if non_block is enabled" if non_block
        if !timeout.is_a?(Float) && !timeout.is_a?(Integer)
          raise TypeError, "no implicit conversion to float from #{timeout.class.to_s.downcase.delete_suffix('class')}"
        end
      end

      @mutex.synchronize do
        begin
          raise ThreadError, 'queue empty' if non_block && @queue.empty?
          return @queue.shift if !@queue.empty? || @closed

          if timeout.nil?
            while @queue.empty? && !@closed
              @waiting << Thread.current unless @waiting.include?(Thread.current)
              @mutex.sleep
            end
          else
            @waiting << Thread.current
            @mutex.sleep(timeout) if @queue.empty? && !@closed
          end
          @queue.shift
        ensure
          @waiting.delete(Thread.current)
        end
      end
    end
    alias deq pop
    alias shift pop

    def push(obj)
      thread = nil
      @mutex.synchronize do
        raise ClosedQueueError, 'queue closed' if @closed

        @queue.push(obj)
        if (thread = @waiting.pop)
          Thread.pass until thread.status == 'sleep'
          thread.wakeup
        end
      end
      self
    end
    alias << push
    alias enq push
  end
end

Queue = Thread::Queue
