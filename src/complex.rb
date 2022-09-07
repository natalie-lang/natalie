class Complex
  alias imag imaginary

  def to_i
    imaginary = self.imaginary

    if (imaginary.respond_to?(:to_i) && imaginary.to_i != 0) || imaginary.is_a?(Float)
      raise RangeError, "can't convert #{self.inspect} to Integer"
    end

    self.real.to_i
  end

  def to_f
    imaginary = self.imaginary

    if (imaginary.is_a?(Float) && imaginary == 0.0) || (imaginary.respond_to?(:to_f) && imaginary.to_f != 0.0)
      raise RangeError, "can't convert #{self.inspect} to Float"
    end

    self.real.to_f
  end

  def to_s
    real = self.real
    imaginary = self.imaginary

    s = ""
    
    if real.respond_to?(:nan?) && real.nan?
      s << "NaN"
    else
      s << real.to_s
    end

    if ! imaginary.negative?
      s << "+"
    end

    if imaginary.respond_to?(:nan?) && imaginary.nan?
      if imaginary.negative?
        s << '-'
      end

      s << "NaN*"
    elsif ! imaginary.finite?
      if imaginary.negative?
        s << '-'
      end

      s << "Infinity*"
    else
      s << imaginary.to_s
    end

    s << "i"
    s
  end

  def finite?
    self.real.finite? && self.imaginary.finite?
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

  def *(other)    
    # (a + bi) * (c + di) = (ac - bd) + (ad + bc)i
    if other.is_a?(Complex)
      ac = self.real * other.real
      bd = self.imaginary * other.imaginary
      ad = self.real * other.imaginary
      bc = self.imaginary * other.real

      return Complex(ac - bd, ad + bc)
    end

    if other.is_a?(Numeric)
      return Complex(self.real * other, self.imaginary * other)
    end

    # NATFIXME: Handle coercion of objects.
  end

  def -@
    Complex(-self.real, -self.imaginary)
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

  def conjugate
    Complex(self.real, -self.imaginary)
  end
  alias conj conjugate

  def abs
    # r = |z| = sqrt(a^2 + b^2).
    Math.sqrt(self.real ** 2 + self.imaginary ** 2)
  end
  alias magnitude abs

  def abs2
    (self.real ** 2) + (self.imaginary ** 2)
  end

  def arg
    # θ = tan^-1(b / a)
    Math.atan2(self.imaginary, self.real)
  end
  alias angle arg
  alias phase arg

  def polar
    # Given z = a + bi, the polar form can be represented as z = r (cosθ + i sinθ)
    # where r = self.abs and θ = self.arg.
    [self.abs, self.arg]
  end

  def self.polar(abs, arg = 0)
    if abs.nil? || arg.nil?
      raise TypeError, "not a real"
    end

    # real = rcosθ
    real = abs * Math.cos(arg)

    # imaginary = rsinθ
    imaginary = abs * Math.sin(arg)

    return Complex(real, imaginary)
  end

  def rect
    self.rectangular
  end

  def rectangular
    return [self.real, self.imaginary]
  end

  def self.rectangular(real, imaginary = 0)
    if ! real.is_a?(Numeric) || ! imaginary.is_a?(Numeric)
      raise TypeError, "not a Numeric"
    end

    if ! real.real? || ! imaginary.real?
      raise TypeError, "not a real"
    end

    Complex(real, imaginary)
  end

  def self.rect(real, imaginary = 0)
    self.rectangular(real, imaginary)
  end

  def to_c
    self
  end

  def real?
    false
  end

  def inspect
    "(#{self.to_s})"
  end

  def coerce(other)
    if other.is_a?(Complex)
      return [other, self]
    end

    if other.is_a?(Numeric) && other.real?
      return [Complex(other), self]
    end

    raise TypeError, "#{other.inspect} can't be coerced into Complex"
  end
end
