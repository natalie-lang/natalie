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
