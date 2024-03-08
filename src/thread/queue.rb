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
  end

  def clear
    @mutex.synchronize { @queue.clear }
    self
  end

  def close
    @mutex.synchronize { @closed = true }
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

  def push(obj)
    @mutex.synchronize do
      raise ClosedQueueError, 'queue closed' if @closed

      @queue.push(obj)
    end
    self
  end
  alias << push
  alias enq push
end
