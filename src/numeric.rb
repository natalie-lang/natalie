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
    raise ZeroDivisionError, 'divided by 0' if other.zero?
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
end
