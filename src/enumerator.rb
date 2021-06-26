class Enumerator
  include Enumerable

  class Yielder
    def yield(item)
      Fiber.yield(item)
    end

    alias << yield
  end

  def initialize(&block)
    @block = block
  end

  def each
    rewind
    final = []
    loop do
      begin
        result = self.next
        final << yield(result)
      rescue StopIteration
        break
      end
    end
    final
  end

  def next
    rewind unless @fiber
    if @peeked
      @peeked = false
      return @peeked_value
    end
    result = @fiber.resume
    if @fiber.status == :terminated
      raise StopIteration, 'iteration reached an end'
    end
    result
  end

  def peek
    if @peeked
      return @peeked_value
    end
    @peeked_value = self.next
    @peeked = true
    @peeked_value
  end

  def rewind
    yielder = Yielder.new
    @fiber = Fiber.new do
      @block.call yielder
    end
  end
end
