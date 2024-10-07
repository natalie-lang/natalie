class Integer
  class << self
    def try_convert(n)
      if n.is_a?(Integer)
        n
      elsif n.respond_to?(:to_int)
        n.to_int.tap do |result|
          if result && !result.is_a?(Integer)
            raise TypeError, "can't convert #{n.class} to Integer (#{n.class}#to_int gives #{result.class})"
          end
        end
      end
    end
  end

  def digits(radix = 10)
    if self < 0
      raise Math::DomainError, 'out of domain'
    end

    if !radix.is_a?(Integer) && radix.respond_to?(:to_int)
      radix = radix.to_int
    end

    unless radix.is_a?(Integer)
      raise TypeError, "no implicit conversion of #{radix.class} into Integer"
    end

    if radix < 0
      raise ArgumentError, 'negative radix'
    elsif radix < 2
      raise ArgumentError, 'invalid radix 0'
    end

    quotient = self
    ary = []
    loop do
      quotient, remainder = quotient.divmod(radix)
      ary << remainder
      break if quotient == 0
    end
    ary
  end

  def div(n)
    if !n.is_a?(Numeric) && n.respond_to?(:coerce)
      a, b = n.coerce(self)
      a.div(b)
    else
      super
    end
  end

  def divmod(other)
    [div(other), self % other]
  end

  def downto(n)
    return enum_for(:downto, n) { self >= n ? (self - n + 1) : 0 } unless block_given?

    i = self
    until i < n
      yield i
      i -= 1
    end
  end

  def fdiv(n)
    if !n.is_a?(Numeric) && n.respond_to?(:coerce)
      a, b = n.coerce(self)
      a.fdiv(b)
    else
      super
    end
  end

  def ceildiv(n)
    -div(-n)
  end

  def gcdlcm(n)
    [gcd(n), lcm(n)]
  end

  def lcm(n)
    (self * n).abs / gcd(n)
  end

  def upto(n)
    return enum_for(:upto, n) { self <= n ? (n - self + 1) : 0 } unless block_given?

    i = self
    until i > n
      yield i
      i += 1
    end
  end

  def allbits?(mask)
    raise TypeError, "No implicit conversion of #{mask.class} into Integer" unless mask.respond_to?(:to_int)

    int_mask = mask.to_int

    unless int_mask.is_a?(Integer)
      raise TypeError, "can't convert #{mask.class} to Integer (#{mask.class}#to_int gives #{int_mask.class})"
    end

    (self & int_mask) == int_mask
  end

  def anybits?(mask)
    raise TypeError, "No implicit conversion of #{mask.class} into Integer" unless mask.respond_to?(:to_int)

    int_mask = mask.to_int

    unless int_mask.is_a?(Integer)
      raise TypeError, "can't convert #{mask.class} to Integer (#{mask.class}#to_int gives #{int_mask.class})"
    end

    (self & int_mask) != 0
  end

  def nobits?(mask)
    raise TypeError, "No implicit conversion of #{mask.class} into Integer" unless mask.respond_to?(:to_int)

    int_mask = mask.to_int

    unless int_mask.is_a?(Integer)
      raise TypeError, "can't convert #{mask.class} to Integer (#{mask.class}#to_int gives #{int_mask.class})"
    end

    (self & int_mask) == 0
  end

  def integer?
    true
  end

  def rationalize(_ = nil)
    to_r
  end

  def remainder(other)
    remainder = self % other
    return remainder if remainder == 0
    return remainder - other if (self < 0 && other > 0) || (self > 0 && other < 0)
    remainder
  end

  def to_r
    Rational(self)
  end
end
