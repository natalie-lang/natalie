class Queue
  def initialize(enum = nil)
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

  def length
    @mutex.synchronize { @queue.length }
  end
  alias size length
end
