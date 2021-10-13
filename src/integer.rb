class Integer
  def downto(n)
    # NATFIXME: Return enumerator if no block given. We cannot store n yet.

    (self - n + 1).times do |i|
      yield self - i
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
end
