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

  def detect
    each do |item|
      return item if yield(item)
    end
    nil
  end

  def each_with_index(*args)
    index = 0
    if block_given?
      each(*args) do |item|
        yield item, index
        index += 1
      end
      self
    else
      Enumerator.new do |y|
        each(*args) do |item|
          y << [item, index]
          index += 1
        end
      end
    end
  end

  def each_with_object(obj)
    if block_given?
      each do |*items|
        if items.size > 1
          yield items, obj
        else
          yield items.first, obj
        end
      end
      obj
    else
      Enumerator.new do |y|
        each do |*items|
          if items.size > 1
            y << [items, obj]
          else
            y << [items.first, obj]
          end
          obj
        end
      end
    end
  end

  def grep(pattern)
    if block_given?
      ary = []
      each do |*item|
        item = items.size > 1 ? items : items[0]
        if pattern === item
          ary << yield(item)
        end
      end
      ary
    else
      select do |item|
        pattern === item
      end
    end
  end

  def grep_v(pattern)
    if block_given?
      ary = []
      each do |*item|
        item = items.size > 1 ? items : items[0]
        if !(pattern === item)
          ary << yield(item)
        end
      end
      ary
    else
      select do |item|
        !(pattern === item)
      end
    end
  end

  def group_by
    raise ArgumentError, 'Support #group_by without block' unless block_given?

    result = Hash.new
    each do |*items|
      item = items.size > 1 ? items : items[0]
      group = yield item
      if result.include? group
        result[group] << item
      else
        result[group] = [item]
      end
    end

    result
  end

  def select
    ary = []
    each do |item|
      ary << item if yield(item)
    end
    ary
  end

  def map
    ary = []
    each do |*items|
      if items.size > 1
        ary << yield(*items)
      else
        ary << yield(items.first)
      end
    end
    ary
  end

  def take(count)
    if not count.is_a? Integer and count.respond_to? :to_int
      count = count.to_int
    end
    raise TypeError unless count.is_a? Integer
    raise ArgumentError,
      'Attempted to take a negative number of values' unless count >= 0

    result = []
    return result if count == 0

    each do |*items|
      item = items.size > 1 ? items : items[0]
      result << item

      break if result.size == count
    end

    result
  end

  def take_while
    has_block = block_given?
    raise ArgumentError, 'called without a block' unless has_block

    result = []
    broken_value = each do |item|
      break self unless yield item
      result << item
    end

    return result if broken_value.equal? self
    broken_value
  end

  def to_a(*args)
    result = []
    each(*args) do |x|
      result << x
    end
    result
  end

  def zip(*args)
    has_block = block_given?
    args = args.map do |arg|
      if arg.respond_to? :to_ary
        arg.to_ary
      elsif arg.respond_to? :to_enum
        arg.to_enum :each
      else
        arg
      end
    end

    all_array = true
    args.each do |arg|
      all_array = arg.is_a? Array
      break if not all_array
    end

    raise 'Support non-array args for #zip' unless all_array

    result = has_block ? nil : []

    each_with_index do |item, index|
      entry = [item]
      if all_array
        args.each do |arg|
          entry << arg[index]
        end
      end
      if has_block
        yield entry
      else
        result << entry
      end
    end
    result
  end
end
