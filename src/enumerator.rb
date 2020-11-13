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
    yielder = Yielder.new
    fiber = Fiber.new do
      @block.call yielder
    end
    loop do
      begin
        result = fiber.resume
        yield result unless fiber.status == :terminated
      rescue FiberError
        break
      end
    end
  end
end
