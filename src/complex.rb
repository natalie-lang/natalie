class Complex
  alias imag imaginary

  def to_i
    imaginary = self.imaginary

    if (imaginary.respond_to?(:to_i) && imaginary.to_i != 0) || imaginary.is_a?(Float)
      raise RangeError, "can't convert #{self.inspect} to Integer"
    end

    self.real.to_i
  end

  def real?
    false
  end
end
