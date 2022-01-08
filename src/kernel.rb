module Kernel
  def <=>(other)
    0 if other.object_id == self.object_id || (!is_a?(Comparable) && self == other)
  end

  def enum_for(method = :each, *args, &block)
    enum =
      Enumerator.new do |yielder|
        the_proc = yielder.to_proc || ->(*i) { yielder.yield(*i) }
        send(method, *args, &the_proc)
      end
    if block_given?
      enum.instance_variable_set(:@size_block, block)
      def enum.size
        @size_block.call
      end
    end
    enum
  end
  alias to_enum enum_for

  def rand(*args)
    Random::DEFAULT.rand(*args)
  end
end
