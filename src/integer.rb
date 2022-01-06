class Integer
  def downto(n)
    unless block_given?
      return enum_for(:downto, n) { self >= n ? (self - n + 1) : 0 }
    end

    i = self
    until i < n
      yield i
      i -= 1
    end
  end

  def upto(n)
    unless block_given?
      return enum_for(:upto, n) { self <= n ? (n - self + 1) : 0 }
    end

    i = self
    until i > n
      yield i
      i += 1
    end
  end

  def allbits?(mask)
    unless mask.respond_to?(:to_int)
      raise TypeError, "No implicit conversion of #{mask.class} into Integer"
    end

    int_mask = mask.to_int

    unless int_mask.is_a?(Integer)
      raise TypeError, "can't convert #{mask.class} to Integer (#{mask.class}#to_int gives #{int_mask.class})"
    end

    (self & int_mask) == int_mask
  end

  def anybits?(mask)
    unless mask.respond_to?(:to_int)
      raise TypeError, "No implicit conversion of #{mask.class} into Integer"
    end

    int_mask = mask.to_int

    unless int_mask.is_a?(Integer)
      raise TypeError, "can't convert #{mask.class} to Integer (#{mask.class}#to_int gives #{int_mask.class})"
    end

    (self & int_mask) != 0
  end

  def nobits?(mask)
    unless mask.respond_to?(:to_int)
      raise TypeError, "No implicit conversion of #{mask.class} into Integer"
    end

    int_mask = mask.to_int

    unless int_mask.is_a?(Integer)
      raise TypeError, "can't convert #{mask.class} to Integer (#{mask.class}#to_int gives #{int_mask.class})"
    end

    (self & int_mask) == 0       
  end

  def integer?
    true
  end
end
