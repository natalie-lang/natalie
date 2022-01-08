class Enumerator
  include Enumerable

  class Chain < Enumerator
    def initialize(*enumerables)
      @current_feed = []
      @enum_block = Proc.new { |yielder| enumerables.each { |enumerable| enumerable.each { |item| yielder << item } } }
    end
  end

  class Yielder
    def initialize(block = nil, appending_args = [])
      @block = block
      @appending_args = appending_args
    end

    def yield(*item)
      Fiber.yield(item)
    end

    alias << yield

    def to_proc
      ->(*args) { @block.call(*args.concat(@appending_args)) } if @block
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
    @fiber = Fiber.new { @enum_block.call @yielder }

    loop do
      begin
        yield self.next
      rescue StopIteration
        return @final_result
      end
    end
  end

  def feed(arg)
    raise TypeError, 'feed value already set' unless @current_feed.empty?
    @current_feed = [arg]
    nil
  end

  def next
    if @peeked
      @peeked = false
      return @peeked_value
    end

    gather = ->(i) { i.size <= 1 ? i.first : i }
    @last_result = gather.(self.next_values)
  end

  def next_values
    unless @fiber
      @yielder = Yielder.new
      @fiber = Fiber.new { @enum_block.call @yielder }
    end

    raise_stop_iteration = -> { raise StopIteration, 'iteration reached and end' }

    raise_stop_iteration.() if @fiber.status == :terminated

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
    return @peeked_value if @peeked
    @peeked_value = self.next
    @peeked = true
    @peeked_value
  end

  def rewind
    @yielder = Yielder.new
    @fiber = Fiber.new { @enum_block.call @yielder }
    @current_feed = []
    self
  end

  def size
    @size
  end

  def with_index(offset = 0)
    offset = 0 if offset == nil

    # When no block is given, ruby checks the argument when executing
    # the enumerator (e.g. when calling #to_a).
    convert_offset = -> do
      if offset.respond_to?(:to_int)
        offset = offset.to_int
      else
        raise TypeError, "no implicit conversion of #{offset.class.name} into Integer"
      end
    end

    if block_given?
      convert_offset.()
      each do |item|
        yield item, offset
        offset += 1
      end
      self
    else
      Enumerator.new do |yielder|
        convert_offset.()
        the_proc = yielder.to_proc || ->(*i) { yielder.yield(*i) }

        each do |item|
          the_proc.(item, offset)
          offset += 1
        end
      end
    end
  end

  class Lazy < Enumerator
    def initialize(obj, size = nil, &block)
      raise ArgumentError, 'tried to call lazy new without a block' unless block_given?

      @obj = obj
      enum = @obj.is_a?(Enumerator) ? @obj : @obj.to_enum
      enum_block = ->(yielder) do
        enum.rewind
        loop { block.call(yielder, *enum.next_values) }
      end

      super(size, &enum_block)
    end

    def chunk(&block)
      return to_enum(:chunk) unless block

      enum_block = ->(yielder) { super(&block).each { |item| yielder << item } }
      lazy = Lazy.new(self) {}
      lazy.instance_variable_set(:@enum_block, enum_block)
      lazy
    end

    def chunk_while(&block)
      raise ArgumentError, 'tried to create Proc object without a block' unless block_given?

      enum_block = ->(yielder) { super(&block).each { |item| yielder << item } }
      lazy = Lazy.new(self) {}
      lazy.instance_variable_set(:@enum_block, enum_block)
      lazy
    end

    def drop(n)
      size = @size ? [0, @size - n].max : nil

      Lazy.new(self, size) do |yielder, *element|
        if n == 0
          yielder.yield(*element)
        else
          n -= 1
        end
      end
    end

    def drop_while(&block)
      raise ArgumentError, 'tried to call lazy drop_while without a block' unless block_given?

      drop = true
      Lazy.new(self) do |yielder, *elements|
        drop = false unless block.call(*elements)

        yielder.yield(*elements) unless drop
      end
    end

    def eager
      Enumerator.new(@size) do |yielder|
        the_proc = yielder.to_proc || ->(*i) { yielder.yield(*i) }
        self.each(&the_proc)
      end
    end

    def filter_map(&block)
      raise ArgumentError, 'tried to call lazy filter_map without a block' unless block_given?

      Lazy.new(self) do |yielder, *element|
        result = block.call(*element)
        yielder << result if result
      end
    end

    def flat_map(&block)
      raise ArgumentError, 'tried to call lazy flat_map without a block' unless block_given?

      Lazy.new(self) do |yielder, *element|
        element = block.call(*element)
        element = element.force if element.respond_to?(:each) && element.respond_to?(:force)

        if element.is_a?(Array) || element.respond_to?(:to_ary)
          element.each { |item| yielder << item }
        else
          yielder << element
        end
      end
    end
    alias collect_concat flat_map

    def grep(pattern, &block)
      process = ->(item) { block ? block.call(item) : item }
      Lazy.new(self) do |yielder, *item|
        item = item.size > 1 ? item : item[0]
        yielder << process.(item) if pattern === item
      end
    end

    def grep_v(pattern, &block)
      process = ->(item) { block ? block.call(item) : item }
      Lazy.new(self) do |yielder, *item|
        item = item.size > 1 ? item : item[0]
        yielder << process.(item) unless pattern === item
      end
    end

    def map(&block)
      raise ArgumentError, 'tried to call lazy select without a block' unless block_given?

      Lazy.new(self, @size) { |yielder, *element| yielder << block.call(*element) }
    end
    alias collect map

    def reject(&block)
      raise ArgumentError, 'tried to call lazy reject without a block' unless block_given?

      gather = ->(item) { item.size <= 1 ? item.first : item }

      Lazy.new(self) do |yielder, *values|
        item = gather.(values)
        yielder.yield(*item) unless block.call(item)
      end
    end

    def select(&block)
      raise ArgumentError, 'tried to call lazy select without a block' unless block_given?

      gather = ->(item) { item.size <= 1 ? item.first : item }

      Lazy.new(self) do |yielder, *values|
        item = gather.(values)
        yielder.yield(*item) if block.call(item)
      end
    end
    alias filter select
    alias find_all select

    def slice_after(*args, &block)
      if block
        raise ArgumentError, "wrong number of arguments (given #{args.size}, expected 0)" if !args.empty?
      elsif args.size != 1
        raise ArgumentError, "wrong number of arguments (given #{args.size}, expected 1)"
      end

      enum_block = ->(yielder) { super(*args, &block).each { |item| yielder << item } }
      lazy = Lazy.new(self) {}
      lazy.instance_variable_set(:@enum_block, enum_block)
      lazy
    end

    def slice_before(*args, &block)
      if block
        raise ArgumentError, "wrong number of arguments (given #{args.size}, expected 0)" if !args.empty?
      elsif args.size != 1
        raise ArgumentError, "wrong number of arguments (given #{args.size}, expected 1)"
      end

      enum_block = ->(yielder) { super(*args, &block).each { |item| yielder << item } }
      lazy = Lazy.new(self) {}
      lazy.instance_variable_set(:@enum_block, enum_block)
      lazy
    end

    def slice_when(&block)
      enum_block = ->(yielder) { super(&block).each { |item| yielder << item } }
      lazy = Lazy.new(self) {}
      lazy.instance_variable_set(:@enum_block, enum_block)
      lazy
    end

    def take(n)
      index = 0
      size = @size ? [n, @size].min : n

      enum_block = ->(yielder) do
        size.times do
          value = self.next_values
          yielder.yield(*value)
        end
      end

      lazy = Lazy.new(self, size) {}
      lazy.instance_variable_set(:@enum_block, enum_block)
      lazy
    end

    def take_while(&block)
      raise ArgumentError, 'tried to call lazy take_while without a block' unless block_given?

      pass = true
      enum_block = ->(yielder) do
        loop do
          elements = self.next_values
          break unless block.call(*elements)
          yielder.yield(*elements)
        end
      end

      lazy = Lazy.new(self) {}
      lazy.instance_variable_set(:@enum_block, enum_block)
      lazy
    end

    def to_enum(method = :each, *args, &block)
      enum_block = ->(yielder) do
        the_proc = yielder.to_proc || ->(*i) { yielder.yield(*i) }
        send(method, *args, &the_proc)
      end

      lazy = Lazy.new(self) {}
      lazy.instance_variable_set(:@enum_block, enum_block)
      if block_given?
        lazy.instance_variable_set(:@size_block, block)
        def lazy.size
          @size_block.call
        end
      end
      lazy
    end
    alias enum_for to_enum

    def with_index(offset = 0, &block)
      offset = 0 if offset == nil

      Lazy.new(self, size) do |yielder, value|
        if offset.respond_to?(:to_int)
          offset = offset.to_int
        else
          raise TypeError, "no implicit conversion of #{offset.class.name} into Integer"
        end

        if block
          block.call(value, offset)
          yielder.yield(value)
        else
          yielder.yield(value, offset)
        end
        offset += 1
      end
    end

    def uniq(&block)
      enum_block = ->(yielder) do
        visited = {}
        loop do
          element = self.next
          visiting = block ? block.call(element) : element

          yielder.yield(*element) unless visited.include?(visiting)

          # We just care about the keys
          visited[visiting] = 0
        end
      end

      lazy = Lazy.new(self) {}
      lazy.instance_variable_set(:@enum_block, enum_block)
      lazy
    end

    def zip(*args, &block)
      if block_given?
        super(*args, &block)
      else
        args.each do |arg|
          arg = arg.to_ary if arg.respond_to? :to_ary

          raise TypeError, "wrong argument type #{arg.class.name} (must respond to :each)" unless arg.respond_to?(:each)
        end

        enum_block = ->(yielder) { super(*args) { |item| yielder << item } }
        lazy = Lazy.new(self, size) {}
        lazy.instance_variable_set(:@enum_block, enum_block)
        lazy
      end
    end

    def lazy
      self
    end

    alias force to_a

    private

    # Add #with_index if implemented in Enumerator
    %i[
      collect
      collect_concat
      drop
      drop_while
      filter
      filter_map
      find_all
      flat_map
      grep
      grep_v
      map
      reject
      select
      take
      take_while
      uniq
      zip
    ].each { |meth| alias_method "_enumerable_#{meth}", meth }
  end
end
