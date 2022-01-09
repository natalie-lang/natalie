module Numeric
  def coerce(other)
    self.class == other.class ? [other, self] : [Float(other), Float(self)]
  end
end
