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

  def none?(pattern = nil)
    gather = ->(item) { item.size <= 1 ? item.first : item }
    if pattern
      each do |*item|
        return false if pattern === gather.(item)
      end
    elsif block_given?
      each do |*item|
        return false if yield(*item)
      end
    else
      each do |*item|
        return false if gather.(item)
      end
    end
    true
  end

  def one?(pattern = nil)
    gather = ->(item) { item.size <= 1 ? item.first : item }
    result = false
    if pattern
      each do |*item|
        if pattern === gather.(item)
          return false if result
          result = true
        end
      end
      result
    elsif block_given?
      each do |*item|
        if yield(*item)
          return false if result
          result = true
        end
      end
      result
    else
      each do |*item|
        if gather.(item)
          return false if result
          result = true
        end
      end
      result
    end
  end

  def chain(*enums)
    Enumerator::Chain.new(self, *enums)
  end

  def chunk
    return enum_for(:chunk) unless block_given?

    Enumerator.new do |yielder|
      last_block_result = nil
      last_chunk = []
      each do |item|
        block_result = yield item
        if block_result.nil? || block_result == :_separator
          last_block_result = block_result
        elsif block_result == :_alone
          yielder << last_chunk unless last_chunk.empty?
          last_chunk = [block_result, [item]]
          last_block_result = block_result
        elsif block_result.is_a?(Symbol) && block_result.start_with?('_')
          raise RuntimeError, 'symbols beginning with an underscore are reserved'
        elsif block_result != last_block_result
          yielder << last_chunk unless last_chunk.empty?
          last_chunk = [block_result, [item]]
          last_block_result = block_result
        else
          last_chunk.last << item
        end
      end
      yielder << last_chunk
    end
  end

  def chunk_while
    raise ArgumentError, 'tried to create Proc object without a block' unless block_given?

    Enumerator.new do |yielder|
      last_chunk = []
      first = true
      last_item = nil
      each do |item|
        if first
          last_item = item
          last_chunk << item
          first = false
          next
        end
        if yield last_item, item
          last_chunk << item
        else
          yielder << last_chunk
          last_chunk = [item]
        end
        last_item = item
      end
      yielder << last_chunk
    end
  end

  def detect
    each do |item|
      return item if yield(item)
    end
    nil
  end

  def drop(n)
    if n.respond_to?(:to_int)
      n = n.to_int
    else
      raise TypeError, "no implicit conversion of #{n.class.name} into Integer"
    end

    raise ArgumentError, 'attempt to drop negative size' if n < 0

    ary = []
    each do |item|
      if n == 0
        ary << item
      else
        n -= 1
      end
    end

    ary
  end

  def drop_while
    return enum_for(:drop_while) unless block_given?

    ary = []

    e = enum_for(:each)
    while true
      begin
        item = e.next
      rescue StopIteration
        return []
      end
      unless yield(item)
        ary << item
        break
      end
    end


    while true
      begin
        ary << e.next
      rescue StopIteration
        break
      end
    end

    ary
  end

  def each_cons(n)
    n = n.to_int if n.respond_to?(:to_int)

    raise ArgumentError, 'invalid size' if n <= 0

    unless block_given?
      return enum_for(:each_cons, n) do
        if n.is_a?(Integer) && respond_to?(:size) && size
          [size - n + 1, 0].max
        else
          Float::INFINITY
        end
      end
    end

    cons = []
    e = enum_for(:each)
    live = true
    while live
      begin
        item = e.next
      rescue StopIteration
        live = false
      end
      if live
        cons << item
        if cons.size >= n
          yield cons
          cons = cons[1..-1]
        end
      end
    end

    nil
  end

  def each_entry(*args)
    return enum_for(:each_entry, *args) unless block_given?

    gather = ->(item) { item.size <= 1 ? item.first : item }

    each(*args) do |*item|
      yield gather.(item)
    end

    self
  end

  def each_slice(count)
    count = count.to_int
    raise ArgumentError, 'invalid slice size' if count < 1

    unless block_given?
      return enum_for(:each_slice, count) do
        if count.is_a?(Integer) && respond_to?(:size) && size
          (size / count.to_f).ceil
        else
          Float::INFINITY
        end
      end
    end

    slice = []
    each do |*items|
      slice << (items.size == 1 ? items.first : items)
      if slice.size >= count
        yield slice
        slice = []
      end
    end
    yield slice if slice.any?
    nil
  end

  def each_with_index(*args)
    unless block_given?
      return enum_for(:each_with_index, *args)
    end

    index = 0
    gather = ->(item) { item.size <= 1 ? item.first : item }
    each(*args) do |*item|
      yield gather.(item), index
      index += 1
    end
    self
  end

  def each_with_object(obj)
    return enum_for(:each_with_object, obj) unless block_given?

    each do |*items|
      if items.size > 1
        yield items, obj
      else
        yield items.first, obj
      end
    end
    obj
  end

  def find(if_none = nil)
    unless block_given?
      return enum_for(:find, if_none)
    end

    gather = ->(item) { item.size <= 1 ? item.first : item }

    each do |*i|
      item = gather.(i)
      if yield(item)
        return item
      end
    end

    if if_none
      return if_none.()
    end

    nil
  end

  def filter_map
    return enum_for(:filter_map) unless block_given?

    gather = ->(item) { item.size <= 1 ? item.first : item }

    arr = []
    each do |*item|
      item = yield(gather.(item))
      arr << item if item
    end
    arr
  end

  alias detect find

  def grep(pattern)
    if block_given?
      ary = []
      each do |*item|
        item = item.size > 1 ? item : item[0]
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
        item = item.size > 1 ? item : item[0]
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
    return enum_for(:group_by) unless block_given?

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

  def include?(obj)
    each do |*items|
      item = items.size > 1 ? items : items[0]
      if obj == item || item == obj
        return true
      end
    end
    false
  end

  alias member? include?

  def inject(*args)
    gather = ->(item) { item.size <= 1 ? item.first : item }
    if args.size == 0
      enum = enum_for(:each)
      memo = nil
      begin
        memo = enum.next
        loop do
          memo = yield memo, enum.next
        end
      rescue StopIteration
      end
      memo
    elsif args.size == 1
      if block_given?
        memo = args.first
        each do |*item|
          memo = yield memo, gather.(item)
        end
        memo
      elsif args.first.is_a?(Symbol)
        sym = args.first
        reduce do |memo, i|
          memo.send(sym, i)
        end
      else
        raise "don't know how to handle arg: #{args.first.inspect}"
      end
    else
      (memo, sym) = args
      each do |obj|
        memo = memo.send(sym, obj)
      end
      memo
    end
  end

  alias reduce inject

  def lazy(&block)
    enum_size = respond_to?(:size) ? size : nil
    if block_given?
      Enumerator::Lazy.new(self, enum_size, &block)
    else
      Enumerator::Lazy.new(self, enum_size) { |yielder, *values|
        yielder.yield(*values)
      }
    end
  end

  def max(n = nil)
    has_block = block_given?
    cmp = ->(result) {
      raise ArgumentError, "bad result from block" unless result.respond_to?(:>)
      result > 0
    }
    if n
      raise ArgumentError, "negative size (#{n})" if n < 0
      ary = []
      enum = enum_for(:each)
      begin
        ary << enum.next
        loop do
          new_val = enum.next
          if has_block
            if ary.size < n || ary.any? { |v| cmp.(yield(new_val, v)) }
              ary << new_val
              ary = ary.sort.last(n)
            end
          else
            if ary.size < n || ary.any? { |v| cmp.(new_val <=> v) }
              ary << new_val
              ary = ary.sort.last(n)
            end
          end
        end
      rescue StopIteration
      end
      ary.sort.reverse
    else
      val = nil
      enum = enum_for(:each)
      begin
        val = enum.next
        loop do
          new_val = enum.next
          if has_block
            val = new_val if cmp.(yield(new_val, val))
          else
            val = new_val if cmp.(new_val <=> val)
          end
        end
      rescue StopIteration
      end
      val
    end
  end

  def max_by(n = nil)
    return enum_for(:max_by) unless block_given?
    if n
      to_a.sort_by { |a| yield a }.last(n).reverse
    else
      max(n) { |a, b|
        fa = yield(a)
        fb = yield(b)
        fa <=> fb
      }
    end
  end

  def min(n = nil)
    has_block = block_given?
    cmp = ->(result) {
      raise ArgumentError, "bad result from block" unless result.respond_to?(:<)
      result < 0
    }
    if n
      raise ArgumentError, "negative size (#{n})" if n < 0
      ary = []
      enum = enum_for(:each)
      begin
        ary << enum.next
        loop do
          new_val = enum.next
          if has_block
            if ary.size < n || ary.any? { |v| cmp.(yield(new_val, v)) }
              ary << new_val
              ary = ary.sort.last(n)
            end
          else
            if ary.size < n || ary.any? { |v| cmp.(new_val <=> v) }
              ary << new_val
              ary = ary.sort.last(n)
            end
          end
        end
      rescue StopIteration
      end
      ary.sort
    else
      val = nil
      enum = enum_for(:each)
      begin
        val = enum.next
        loop do
          new_val = enum.next
          if has_block
            val = new_val if cmp.(yield(new_val, val))
          else
            val = new_val if cmp.(new_val <=> val)
          end
        end
      rescue StopIteration
      end
      val
    end
  end

  def min_by(n = nil)
    return enum_for(:min_by) unless block_given?
    if n
      to_a.sort_by { |a| yield a }.take(n)
    else
      min(n) { |a, b|
        fa = yield(a)
        fb = yield(b)
        fa <=> fb
      }
    end
  end

  def minmax(&block)
    block_given = block_given?
    gather = ->(*item) { item.size <= 1 ? item.first : item }
    cmp = ->(result) {
      raise ArgumentError, "bad result from block" unless result.respond_to?(:<)
      result < 0
    }

    enumerator = enum_for(:each)

    begin
      min = max = enumerator.next

      loop do
        value = gather.(enumerator.next)
        if block_given
          if cmp.(yield(value, min))
            min = value
          elsif cmp.(yield(max, value))
            max = value
          end
        else
          if cmp.(value <=> min)
            min = value
          elsif cmp.(max <=> value)
            max = value
          end
        end
      end
    rescue StopIteration
    end

    [min, max]
  end

  def minmax_by(&block)
    return enum_for(:minmax_by) unless block_given?

    minmax { |a, b|
      yield(a) <=> yield(b)
    }
  end

  def reject
    return enum_for(:reject) unless block_given?

    ary = []

    gather = ->(obj) { obj.size <= 1 ? obj.first : obj }

    each do |*item|
      ary << gather.(item) unless yield(gather.(item))
    end

    ary
  end

  def reverse_each(*args)
    return enum_for(:reverse_each, *args) unless block_given?

    ary = []
    each(*args) do |*item|
      ary << item
    end

    gather = ->(obj) { obj.size <= 1 ? obj.first : obj }

    ary.reverse.each do |item|
      yield gather.(item)
    end
  end

  def partition
    return enum_for(:partition) unless block_given?

    gather = ->(obj) { obj.size <= 1 ? obj.first : obj }

    left = []
    right = []
    each do |*item|
      if yield(gather.(item))
        left << gather.(item)
      else
        right << gather.(item)
      end
    end
    [left, right]
  end

  def select
    return enum_for(:select) unless block_given?

    ary = []

    gather = ->(obj) { obj.size <= 1 ? obj.first : obj }

    each do |*item|
      ary << gather.(item) if yield(gather.(item))
    end
    ary
  end

  alias find_all select
  alias filter select

  def collect
    return enum_for(:map) unless block_given?

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

  alias map collect

  def collect_concat
    return enum_for(:collect_concat) unless block_given?

    ary = []
    each do |*items|
      if items.size > 1
        result = yield(*items)
      else
        result = yield(items.first)
      end
      if result.respond_to?(:to_ary)
        a = result.to_ary
        case a
        when Array
          ary += a
        when nil
          ary << result
        else
          raise TypeError, 'expected to_ary to return an Array or nil'
        end
      else
        ary << result
      end
    end
    ary
  end

  alias flat_map collect_concat

  def count(*args)
    count = 0
    if args.size > 0
      if block_given?
        $stderr.puts("warning: given block not used")
      end
      gather = ->(obj) { obj.size <= 1 ? obj.first : obj }
      item = args.first
      each do |*obj|
        count += 1 if gather.(obj) == item
      end
    elsif block_given?
      each do |*obj|
        count += 1 if yield(*obj)
      end
    else
      each do
        count += 1
      end
    end
    count
  end

  def cycle(n = nil)
    unless block_given?
      return enum_for(:cycle, n) do
        if n.is_a?(Integer) && respond_to?(:size) && size
          [n * size, 0].max
        else
          Float::INFINITY
        end
      end
    end

    return if n.is_a?(Integer) && n <= 0

    cache = []

    if n
      if n.respond_to?(:to_int)
        n = n.to_int
      else
        raise TypeError, "could not coerce #{n.inspect} to Integer"
      end
    end

    gather = ->(item) { item.size <= 1 ? item.first : item }
    each do |*item|
      item = gather.(item)
      cache << item
      yield item
    end

    n -= 1 if n

    return if cache.empty?

    e = cache.enum_for(:each)
    loop do
      begin
        item = e.next
      rescue StopIteration
        if n
          n -= 1
          break if n <= 0
        end
        e.rewind
        item = e.next
      end
      yield item
    end
  end

  def find_index(*args)
    if args.length == 0 && !block_given?
      return enum_for(:find_index)
    end

    if args.length > 1
      raise ArgumentError, "wrong number of arguments (given #{args.length}, expected 1)"
    end

    block_given = block_given?

    if args.size != 0 && block_given
      block_given = false
      $stderr.puts("warning: given block not used")
    end

    gather = ->(item) { item.size <= 1 ? item.first : item }
    index = 0
    found = false

    each do |*item|
      if block_given
        if yield(*item)
          found = true
          break
        end
      elsif args.first == gather.(item)
        found = true
        break
      end

      index += 1
    end

    found ? index : nil
  end

  def first(*args)
    if args.length == 0
      each do |*item|
        return item.size <= 1 ? item.first : item
      end
    else
      count = args[0]
      if not count.is_a? Integer and count.respond_to? :to_int
        count = count.to_int
      end
      raise TypeError unless count.is_a? Integer
      raise ArgumentError, 'negative array size' unless count >= 0

      result = []
      return result if count == 0

      each do |*items|
        item = items.size > 1 ? items : items[0]
        result << item

        break if result.size == count
      end

      result
    end
  end

  def slice_after(*args, &block)
    current_slice = []
    block_given = block_given?
    gather = ->(item) { item.size <= 1 ? item.first : item }

    if block_given
      if !args.empty?
        raise ArgumentError, "wrong number of arguments (given #{args.size}, expected 0)"
      end
    elsif args.size != 1
      raise ArgumentError, "wrong number of arguments (given #{args.size}, expected 1)"
    end

    condition = ->(item) {
      if block_given
        block.(item)
      else
        args[0] === item
      end
    }

    Enumerator.new do |yielder|
      each do |*item|
        item = gather.(item)
        current_slice << item
        if condition.(item)
          yielder << current_slice
          current_slice = []
        end
      end

      unless current_slice.empty?
        yielder << current_slice
      end
    end
  end

  def slice_before(*args, &block)
    current_slice = []
    block_given = block_given?
    gather = ->(item) { item.size <= 1 ? item.first : item }

    if block_given
      if !args.empty?
        raise ArgumentError, "wrong number of arguments (given #{args.size}, expected 0)"
      end
    elsif args.size != 1
      raise ArgumentError, "wrong number of arguments (given #{args.size}, expected 1)"
    end

    condition = ->(item) {
      if block_given
        block.(item)
      else
        args[0] === item
      end
    }

    Enumerator.new do |yielder|
      index = 0
      each do |*item|
        item = gather.(item)
        if condition.(item) && index > 0
          yielder << current_slice
          current_slice = [item]
        else
          current_slice << item
        end
        index += 1
      end

      unless current_slice.empty?
        yielder << current_slice
      end
    end
  end

  def slice_when(&block)
    block = proc(&block)
    current_slice = []
    enum = to_enum

    Enumerator.new do |yielder|
      index = 0
      loop do
        begin
          a = enum.next
          if index == 0
            current_slice << a
          end
          b = enum.peek
          if block.call(a, b)
            yielder << current_slice
            current_slice = []
          end
          current_slice << b
          index += 1
        rescue StopIteration
          break
        end
      end

      unless current_slice.empty?
        yielder << current_slice
      end
    end
  end

  def sort(&block)
    to_a.sort(&block)
  end

  def sort_by(&block)
    to_a.sort_by!(&block)
  end

  def sum(init = 0)
    block_given = block_given?
    each do |item|
      if block_given
        init += yield item
      else
        init += item
      end
    end
    init
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

  def tally
    hash = {}
    gather = ->(item) { item.size <= 1 ? item.first : item }

    each do |*item|
      hash[gather.(item)] ||= 0
      hash[gather.(item)] += 1
    end

    hash
  end

  def to_a(*args)
    result = []
    gather = ->(item) { item.size <= 1 ? item.first : item }
    each(*args) do |*item|
      result << gather.(item)
    end
    result
  end

  alias entries to_a

  def to_h(*args)
    h = {}

    block_given = block_given? # FIXME: bug in block_given? method

    index = 0
    each(*args) do |*item|
      item = item.first if item.size == 1

      if block_given
        item = yield(item)
      end

      if item.respond_to?(:to_ary)
        item = item.to_ary
      end

      if item.is_a?(Array)
        if item.size == 2
          (key, val) = item
          h[key] = val
        else
          raise ArgumentError, "element has wrong array length at #{index} (expected 2, was #{item.size})"
        end
      else
        raise TypeError, "wrong element type #{item.class.name} at #{index} (expected array)"
      end

      index += 1
    end

    h
  end

  def uniq(&block)
    ary = to_a
    ary.uniq!(&block) || ary
  end

  def zip(*args)
    has_block = block_given?
    args = args.map do |arg|
      if arg.respond_to? :to_ary
        arg.to_ary
      elsif arg.respond_to? :to_enum
        enumerable_value = arg.to_enum :each
        unless enumerable_value.is_a? Array
          enumerable_value = enumerable_value.to_a if enumerable_value.respond_to? :to_a
        end
        enumerable_value
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
