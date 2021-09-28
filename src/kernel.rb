module Kernel
  def enum_for(method = :each, *args)
    size = block_given? ? yield : nil
    Enumerator.new(size) do |yielder|
      the_proc = yielder.to_proc || ->(*i) { yielder.yield(*i) }
      send(method, *args, &the_proc)
    end
  end
  alias to_enum enum_for

  def rand(*args)
    Random::DEFAULT.rand(*args)
  end
end
