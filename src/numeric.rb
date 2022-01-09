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
end
