class Complex
  alias imag imaginary

  def to_i
    imaginary = self.imaginary

    if (imaginary.respond_to?(:to_i) && imaginary.to_i != 0) || imaginary.is_a?(Float)
      raise RangeError, "can't convert #{self.inspect} to Integer"
    end

    self.real.to_i
  end

  def +(other)
    if other.is_a?(Complex)
      return Complex(self.real + other.real, self.imaginary + other.imaginary)
    end

    if other.is_a?(Numeric)
      return Complex(self.real + other, self.imaginary)
    end
  end

  def -(other)
    if other.is_a?(Complex)
      return Complex(self.real - other.real, self.imaginary - other.imaginary)
    end

    if other.is_a?(Numeric)
      return Complex(self.real - other, self.imaginary)
    end
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

  def abs
    # r = |z| = sqrt(a^2 + b^2).
    Math.sqrt(self.real ** 2 + self.imaginary ** 2)
  end

  def arg
    # θ = tan^-1(b / a)
    Math.atan2(self.imaginary, self.real)
  end

  def polar
    # Given z = a + bi, the polar form can be represented as z = r (cosθ + i sinθ)
    # where r = self.abs and θ = self.arg.
    [self.abs, self.arg]
  end

  def to_c
    self
  end

  def real?
    false
  end
end
