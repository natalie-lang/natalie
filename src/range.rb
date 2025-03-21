class Range
  def count(*args, &block)
    if args.empty? && block.nil?
      return Float::INFINITY if self.begin.nil?
      return Float::INFINITY if self.end.nil?
    end
    super(*args, &block)
  end

  def cover?(object)
    greater_than_begin = -> do
      value = object.is_a?(Range) ? object.begin : object
      return false unless value

      compare_result = (self.begin <=> value)
      !!compare_result && compare_result != 1
    end

    less_than_end = -> do
      if object.is_a?(Range)
        object_end =
          if object.exclude_end? && object.end.respond_to?(:pred)
            object.end.pred
          else
            object.end
          end

        compare_result = self.end <=> object_end
        return false unless compare_result

        if exclude_end? && !object.exclude_end?
          compare_result == 1
        else
          compare_result >= 0
        end
      else
        expected = exclude_end? ? -1 : 0
        compare_result = (object <=> self.end)
        !!compare_result && compare_result <= expected
      end
    end

    result = true

    result &&= greater_than_begin.() if self.begin

    result &&= less_than_end.() if self.end

    result
  end
  alias === cover?

  def max(n = nil)
    raise RangeError, 'cannot get the maximum of endless range' unless self.end

    if block_given?
      raise RangeError, 'cannot get the maximum of beginless range with custom comparison method' unless self.begin

      return super
    end

    return last(n) if n

    # If end is not a numeric and the range excludes it's end, we cannot calculate the max value without iterating
    return super if exclude_end? && !self.end.is_a?(Numeric)

    max =
      if exclude_end?
        raise TypeError, 'cannot exclude end value with non Integer begin value' unless self.begin

        if self.end.is_a?(Integer)
          self.end - 1
        else
          raise TypeError, 'cannot exclude non Integer end value'
        end
      else
        self.end
      end

    max if !self.begin || (max == self.begin || (self.begin <=> max) == -1)
  end

  def min(n = nil)
    raise RangeError, 'cannot get the minimum of beginless range' unless self.begin

    if block_given?
      raise RangeError, 'cannot get the minimum of endless range with custom comparison method' unless self.end

      return super
    end

    return first(n) if n

    min = first
    return min if !self.end

    if exclude_end?
      return min if min < self.end
    else
      return min if min == self.end || (min <=> self.end) == -1
    end
  end

  def minmax(&block)
    return super if block_given?

    [min, max]
  end

  def overlap?(other)
    raise TypeError, "wrong argument type #{other.class} (expected Range)" unless other.is_a?(Range)

    is_empty =
      lambda do |arg|
        compare = arg.exclude_end? ? :>= : :>
        !arg.begin.nil? && !arg.end.nil? && arg.begin.__send__(compare, arg.end)
      end

    if is_empty.call(self) || is_empty.call(other)
      false
    elsif (self.begin.nil? && self.end.nil?) || (other.begin.nil? && other.end.nil?)
      true
    elsif self.begin.nil?
      compare = exclude_end? ? :< : :<=
      other.begin.nil? || other.begin.__send__(compare, self.end)
    elsif other.begin.nil?
      compare = other.exclude_end? ? :< : :<=
      self.begin.nil? || self.begin.__send__(compare, other.end)
    elsif self.end.nil?
      compare = other.exclude_end? ? :< : :<=
      other.end.nil? || self.begin.__send__(compare, other.end)
    elsif other.end.nil?
      compare = exclude_end? ? :< : :<=
      self.end.nil? || other.begin.__send__(compare, self.end)
    else
      compare_other_end = other.exclude_end? ? :< : :<=
      compare_self_end = exclude_end? ? :< : :<=
      begin
        self.begin.__send__(compare_other_end, other.end) && other.begin.__send__(compare_self_end, self.end)
      rescue ArgumentError
        false
      end
    end
  end

  def size
    raise TypeError, "can't iterate from #{self.begin.class}" unless self.begin.respond_to?(:succ)
    return if self.begin.nil? && !self.end.is_a?(Numeric)
    return unless self.begin.is_a?(Numeric)
    return Float::INFINITY if self.end.nil?
    return 0 if self.end < self.begin
    return Float::INFINITY if self.end.is_a?(Float) && self.end.infinite?
    size = self.end - self.begin
    size += 1 unless exclude_end?
    size
  end

  def reverse_each
    raise TypeError, "can't iterate from #{self.end.class}" if self.end.nil?
    return super unless self.end.respond_to?(:pred)
    size = self.begin.nil? ? Float::INFINITY : self.size
    return enum_for(:reverse_each) { size } unless block_given?
    item = self.end
    item = item.pred if exclude_end?
    while self.begin.nil? || item >= self.begin
      yield item
      item = item.pred
    end
    self
  end
end
