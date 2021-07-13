module Kernel
  def enum_for(method, *args)
    size = block_given? ? yield : nil
    Enumerator.new(size) do |yielder|
      the_proc = yielder.to_proc || ->(*i) { yielder << (i.size <= 1 ? i.first : i) }
      send(method, *args, &the_proc)
    end
  end
end
