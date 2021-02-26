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
    final = []
    loop do
      begin
        result = fiber.resume
        unless fiber.status == :terminated
          final << yield(result)
        end
      rescue FiberError
        break
      end
    end
    final
  end
end
