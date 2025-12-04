module Comparable
  def ==(other)
    result = self <=> other
    raise ArgumentError, "comparison of #{result.class} with 0 failed" if !result.is_a?(Numeric) && !result.nil?
    result == 0
  end

  def <(other)
    result = (self <=> other)
    raise ArgumentError, "comparison of #{self.class} with #{other.inspect} failed" if result.nil?
    result < 0
  end

  def <=(other)
    result = (self <=> other)
    raise ArgumentError, "comparison of #{self.class} with #{other.inspect} failed" if result.nil?
    result <= 0
  end

  def >(other)
    !(self <= other)
  end

  def >=(other)
    !(self < other)
  end

  def between?(min, max)
    self >= min && self <= max
  end

  def clamp(*args)
    if args.length == 2
      min, max = args

      if !min.nil? && !max.nil?
        compared = min <=> max
        raise ArgumentError, 'min argument must be smaller than max argument' if compared.nil? || compared > 0
      end

      if !min.nil? && self < min
        min
      elsif !max.nil? && self > max
        max
      else
        self
      end
    elsif args.length == 1 && args.first.is_a?(Range)
      range = args.first
      raise ArgumentError, 'cannot clamp with an exclusive range' if !range.end.nil? && range.exclude_end?
      clamp(range.begin, range.end)
    elsif args.length == 1
      raise TypeError, "wrong argument type #{args.first.inspect} (expected Range)"
    else
      raise ArgumentError, "wrong number of arguments (given #{args.length}, expected 1..2)"
    end
  end
end
