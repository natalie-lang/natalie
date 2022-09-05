class Complex
  alias imag imaginary

  def to_i
    imaginary = self.imaginary

    if (imaginary.respond_to?(:to_i) && imaginary.to_i != 0) || imaginary.is_a?(Float)
      raise RangeError, "can't convert #{self.inspect} to Integer"
    end

    self.real.to_i
  end

  def ==(other)
    real = self.real
    imaginary = self.imaginary

    if ! other.is_a?(Complex) && imaginary == 0
      return real == other
    end

    other_real = other.real
    other_imaginary = other.imaginary

    if real == 0 || other_imaginary == 0
      return false
    end

    if real != other_real
      return false
    end

    if imaginary != other_imaginary
      return false
    end

    return true
  end

  def to_c
    self
  end

  def real?
    false
  end
end
