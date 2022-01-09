module Numeric
  def coerce(other)
    self.class == other.class ? [other, self] : [Float(other), Float(self)]
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

  def abs
    self.negative? ? -self : self
  end

  alias magnitude abs

  def abs2
    self * self
  end
end
