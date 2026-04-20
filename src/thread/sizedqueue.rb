class Thread
  class SizedQueue < Queue
    def initialize(max)
      super()
      @max = coerce_max(max)
      @push_waiters = []
    end

    attr_reader :max

    def max=(max)
      new_max = coerce_max(max)

      threads = []
      @mutex.synchronize do
        diff = new_max - @max
        @max = new_max
        threads = @push_waiters.shift(diff) if diff > 0
      end
      threads.each { |t| wake_thread(t) }
      max
    end

    def num_waiting
      @mutex.synchronize { @waiting.length + @push_waiters.length }
    end

    def push(obj, non_block = false, timeout: nil)
      unless timeout.nil?
        raise ArgumentError, "can't set a timeout if non_block is enabled" if non_block
        timeout = coerce_timeout(timeout)
      end

      @mutex.synchronize do
        raise ClosedQueueError, 'queue closed' if @closed

        if @queue.size >= @max
          raise ThreadError, 'queue full' if non_block

          deadline = timeout && Process.clock_gettime(Process::CLOCK_MONOTONIC) + timeout
          @push_waiters << Thread.current
          begin
            while @queue.size >= @max && !@closed
              if deadline
                remaining = deadline - Process.clock_gettime(Process::CLOCK_MONOTONIC)
                break if remaining <= 0
                @mutex.sleep(remaining)
              else
                @mutex.sleep
              end
            end
          ensure
            @push_waiters.delete(Thread.current)
          end

          raise ClosedQueueError, 'queue closed' if @closed
          return nil if @queue.size >= @max
        end

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

    def pop(non_block = false, timeout: nil)
      result = super
      wake_push_waiter
      result
    end
    alias deq pop
    alias shift pop

    def clear
      drain_push_waiters { @queue.clear }
      self
    end

    def close
      super
      drain_push_waiters
      self
    end

    private

    def coerce_timeout(timeout)
      case timeout
      when Integer, Float, Rational
        timeout
      when String, Symbol, TrueClass, FalseClass
        raise TypeError, "no implicit conversion to float from #{timeout.class.to_s.downcase.delete_suffix('class')}"
      else
        unless timeout.respond_to?(:to_f)
          raise TypeError, "no implicit conversion to float from #{timeout.class.to_s.downcase.delete_suffix('class')}"
        end
        coerced = timeout.to_f
        unless coerced.is_a?(Float)
          raise TypeError, "can't convert #{timeout.class} into Float (#{timeout.class}#to_f gives #{coerced.class})"
        end
        coerced
      end
    end

    def coerce_max(max)
      max =
        case max
        when Integer
          max
        when Float
          max.to_i
        when NilClass, TrueClass, FalseClass
          raise TypeError, "no implicit conversion from #{max.inspect} to integer"
        else
          raise TypeError, "no implicit conversion of #{max.class} into Integer" unless max.respond_to?(:to_int)
          coerced = max.to_int
          unless coerced.is_a?(Integer)
            raise TypeError, "can't convert #{max.class} to Integer (#{max.class}#to_int gives #{coerced.class})"
          end
          coerced
        end
      raise ArgumentError, 'queue size must be positive' if max <= 0
      max
    end

    def drain_push_waiters
      threads = nil
      @mutex.synchronize do
        yield if block_given?
        threads = @push_waiters
        @push_waiters = []
      end
      threads.each { |t| wake_thread(t) }
    end

    def wake_push_waiter
      return if @push_waiters.empty?
      thread = nil
      @mutex.synchronize { thread = @push_waiters.shift }
      wake_thread(thread) if thread
    end

    def wake_thread(thread)
      Thread.pass until thread.status == 'sleep' || thread.status == false || thread.status.nil?
      thread.wakeup
    rescue ThreadError
      nil
    end
  end
end

SizedQueue = Thread::SizedQueue
