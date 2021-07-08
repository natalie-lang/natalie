class Enumerator
  include Enumerable

  class Chain < Enumerator
    def initialize(*enumerables)
      @enum_block = Proc.new do |yielder|
        enumerables.each do |enumerable|
          enumerable.each do |item|
            yielder << item
          end
        end
      end
    end
  end

  class Yielder
    def initialize(block = nil)
      @block = block
    end

    def yield(item)
      Fiber.yield(item)
    end

    alias << yield

    def to_proc
      @block
    end
  end

  def initialize(size = nil, &enum_block)
    @size = size
    @enum_block = enum_block
  end

  def each(&block)
    @yielder = Yielder.new(block)
    @fiber = Fiber.new do
      @enum_block.call @yielder
    end
    loop do
      begin
        yield self.next
      rescue StopIteration
        return @final_result
      end
    end
  end

  def next
    rewind unless @fiber
    if @peeked
      @peeked = false
      return @peeked_value
    end
    @last_result = @fiber.resume(@last_result)
    if @fiber.status == :terminated
      @final_result = @last_result
      raise StopIteration, 'iteration reached an end'
    end
    @last_result
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
    @yielder = Yielder.new
    @fiber = Fiber.new do
      @enum_block.call @yielder
    end
  end

  def size
    @size
  end
end
