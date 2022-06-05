class Range
  def count(*args, &block)
    if args.empty? && block.nil?
      return Float::INFINITY if self.begin.nil?
      return Float::INFINITY if self.end.nil?
    end
    super(*args, &block)
  end

  def cover?(object)
    greater_than_begin = ->() {
      value = object.is_a?(Range) ? object.begin : object
      return false unless value

      compare_result = (self.begin <=> value)
      !!compare_result && compare_result != 1
    }

    less_than_end = ->() {
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
    }

    result = true

    if self.end && self.begin
      compare_result = self.begin <=> self.end
      if !compare_result || compare_result == 1 || (exclude_end? && compare_result == 0)
        return false
      end
    end

    if self.begin
      result &&= greater_than_begin.()
    end

    if self.end
      result &&= less_than_end.()
    end

    result
  end
  alias === cover?

  def max(n = nil)
    raise RangeError, 'cannot get the maximum of endless range' unless self.end

    if block_given?
      raise RangeError, 'cannot get the maximum of beginless range with custom comparison method' unless self.begin

      return super
    end

    if n
      return last(n)
    end

    # If end is not a numeric and the range excludes it's end, we cannot calculate the max value without iterating
    if exclude_end? && !self.end.is_a?(Numeric)
      return super
    end

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

    if !self.begin || (max == self.begin || (self.begin <=> max) == -1)
      max
    end
  end

  def min(n = nil)
    raise RangeError, 'cannot get the minimum of beginless range' unless self.begin

    if block_given?
      raise RangeError, 'cannot get the minimum of endless range with custom comparison method' unless self.end

      return super
    end

    if n
      return first(n)
    end

    min = first
    return min if !self.end

    if exclude_end?
      return min if min < self.end
    else
      return min if min == self.end || (min <=> self.end) == -1
    end
  end

  def minmax(&block)
    if block_given?
      return super
    end

    [min, max]
  end

  def size
    return Float::INFINITY if self.begin.nil?
    return unless self.begin.is_a?(Numeric)
    return Float::INFINITY if self.end.nil?
    return unless self.end.is_a?(Numeric)
    return 0 if self.end < self.begin
    return Float::INFINITY if self.begin.is_a?(Float) && self.begin.infinite?
    return Float::INFINITY if self.end.is_a?(Float) && self.end.infinite?
    size = self.end - self.begin
    size += 1 unless exclude_end?
    return size.floor if size.is_a?(Float)
    size
  end
end
