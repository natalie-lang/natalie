require 'natalie/inline'

class Numeric
  include Comparable

  def coerce(other)
    self.class == other.class ? [other, self] : [Float(other), Float(self)]
  end

  def eql?(other)
    self.class == other.class && self == other
  end

  def -@
    minuend, subtrahend = self.coerce(0)
    minuend - subtrahend
  end

  def +@
    self
  end

  def negative?
    self < 0
  end

  def positive?
    self > 0
  end

  def zero?
    self == 0
  end

  def nonzero?
    self.zero? ? nil : self
  end

  def abs
    self.negative? ? -self : self
  end

  alias magnitude abs

  def abs2
    self * self
  end

  def arg
    self.negative? ? Math::PI : 0
  end

  alias angle arg

  alias phase arg

  def polar
    [self.abs, self.arg]
  end

  def conj
    self
  end

  alias conjugate conj

  def imag
    0
  end

  alias imaginary imag

  def real
    self
  end

  def real?
    true
  end

  def rect
    [self.real, self.imag]
  end

  alias rectangular rect

  def ceil
    Float(self).ceil
  end

  def floor
    Float(self).floor
  end

  def round
    Float(self).round
  end

  def truncate
    Float(self).truncate
  end

  def clone(freeze: nil)
    raise ArgumentError, "can't unfreeze #{self.class}" if freeze == false
    self
  end

  def dup
    self
  end

  def div(other)
    raise ZeroDivisionError, 'divided by 0' if other == 0
    (self / other).floor
  end

  def %(other)
    self - other * self.div(other)
  end

  alias modulo %

  def divmod(other)
    [self.div(other), self % other]
  end

  def remainder(other)
    remainder = self % other
    return remainder if remainder == 0
    return remainder - other if (self < 0 && other > 0) || (self > 0 && other < 0)
    remainder
  end

  def fdiv(other)
    Float(self) / Float(other)
  end

  def finite?
    true
  end

  def infinite?
    nil
  end

  def integer?
    false
  end

  def to_int
    self.to_i
  end

  __function__('new Enumerator::ArithmeticSequenceObject', %w[Value Value Value bool], 'Value')

  def step(to_pos = nil, by_pos = nil, by: nil, to: nil)
    if to_pos && to
      raise ArgumentError, 'to is given twice'
    end

    if by_pos && by
      raise ArgumentError, 'by is given twice'
    end

    by ||= by_pos || 1
    to ||= to_pos

    unless block_given?
      return __call__('new Enumerator::ArithmeticSequenceObject', self, to, by, false)
    end

    if !by.is_a?(Numeric) && by.respond_to?(:to_int)
      by = by.to_int
    end

    # This is here for compatiblity with ruby.
    # When `by` is a non-numeric that cannot be compared with zero, this
    # should raise an ArgumentError.
    ascending = by > 0

    if !by.is_a?(Numeric)
      raise TypeError, "no implicit conversion of #{by.class} into Integer"
    end

    if by == 0
      raise ArgumentError, 'step can\'t be 0'
    end

    # Special handling for infinite values
    if by.is_a?(Float) && by.infinite?
      if ascending ? self <= to : self >= to
        yield to_f
      end
      return self
    end

    if to.is_a?(Float) && to.infinite?
      return self
    end

    # If any argument is a float, we want to use floats from here on
    if is_a?(Float) || by.is_a?(Float) || to.is_a?(Float)
      by = by.to_f
      to = to.to_f
    end

    # Calculate how many times we have to iterate a step
    step_count =
      if by.is_a?(Integer)
        n = to - self
        ascending ? n : -n
      else
        n = (to - self) / by
        (n + n * Float::EPSILON).floor
      end

    if step_count.negative?
      return self
    end

    step_count += 1

    # Execute the steps
    step_count.times do |index|
      value = (by * index) + self

      # Ensure that we do not yield a number that exceeds `to`
      if !ascending ? value < to : value > to
        value = to
      end

      yield value
    end

    self
  end
end
