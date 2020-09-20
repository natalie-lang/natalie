module Enumerable
  def all?(pattern = nil)
    gather = ->(item) { item.size <= 1 ? item.first : item }
    if pattern
      each do |*item|
        return false unless pattern === gather.(item)
      end
      true
    elsif block_given?
      each do |*item|
        return false unless yield(*item)
      end
      true
    else
      each do |*item|
        return false unless gather.(item)
      end
      true
    end
  end

  def any?(pattern = nil)
    gather = ->(item) { item.size <= 1 ? item.first : item }
    if pattern
      each do |*item|
        return true if pattern === gather.(item)
      end
      false
    elsif block_given?
      each do |*item|
        return true if yield(*item)
      end
      false
    else
      each do |*item|
        return true if gather.(item)
      end
      false
    end
  end

  def each_with_index(*args)
    index = 0
    if block_given?
      # FIXME: Expose ArgumentError.
      raise "Wrong number of arguments, given #{args.size} expected 0" unless args.empty?
      each do |item|
        yield item, index
        index += 1
      end
      self
    else
      raise 'Support #each_with_index without block'
    end
  end

  def grep(pattern)
    if block_given?
      ary = []
      each do |*item|
        if item.size <= 1
          if pattern === item.first
            ary << yield(*item)
          end
        else
          if pattern === item
            ary << yield(item)
          end
        end
      end
      ary
    else
      select do |item|
        pattern === item
      end
    end
  end

  def select
    ary = []
    each do |item|
      ary << item if yield(item)
    end
    ary
  end
end
