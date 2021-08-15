class Enumerator
  include Enumerable

  class Chain < Enumerator
    def initialize(*enumerables)
      @current_feed = []
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
    def initialize(block = nil, appending_args = [])
      @block = block
      @appending_args = appending_args
    end

    def yield(item = nil)
      Fiber.yield(item)
    end

    alias << yield

    def to_proc
      if @block
        -> (*args) {
          @block.call(*args.concat(@appending_args))
        }
      end
    end
  end

  def initialize(size = nil, &enum_block)
    @size = size
    @enum_block = enum_block
    @current_feed = []
  end

  def each(*appending_args, &block)
    unless block_given?
      if appending_args.empty?
        return self
      else
        return enum_for(:each, *appending_args)
      end
    end

    @yielder = Yielder.new(block, appending_args)
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

  def feed(arg)
    unless @current_feed.empty?
      raise TypeError, 'feed value already set'
    end
    @current_feed = [arg]
    nil
  end

  def next
    unless @fiber
      @yielder = Yielder.new
      @fiber = Fiber.new do
        @enum_block.call @yielder
      end
    end

    if @peeked
      @peeked = false
      return @peeked_value
    end

    raise_stop_iteration = ->() {
      raise StopIteration, 'iteration reached and end'
    }

    if @fiber.status == :terminated
      raise_stop_iteration.()
    end

    begin
      yield_return_value = @last_result
      if !@current_feed.empty? && @fiber.status != :created
        yield_return_value = @current_feed[0]
        @current_feed = []
      end

      @last_result = @fiber.resume(yield_return_value)
    rescue Exception => e
      rewind
      raise e
    end

    if @fiber.status == :terminated
      @final_result = @last_result
      raise_stop_iteration.()
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
    @current_feed = []
    self
  end

  def size
    @size
  end
end
