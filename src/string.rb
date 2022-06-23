class String
  alias replace initialize_copy

  def upto(other_string, exclusive = false)
    return enum_for(:upto, other_string, exclusive) unless block_given?

    other_string = if other_string.is_a?(String)
                     other_string
                   elsif other_string.respond_to?(:to_str)
                     other_string.to_str
                   end

    unless other_string.is_a?(String)
      raise TypeError, "no implicit conversion of #{other_string.class} into String"
    end

    nums = /^\d+$/
    if self =~ nums && other_string =~ nums
      treat_as_int_succ = true
      return self if other_string.to_i < self.to_i
    else
      return self if other_string < self
      return self if other_string.size < self.size
    end

    special_succ = ->(s) do
      if treat_as_int_succ
        # special handling since both ends can be ints
        s.to_i.succ.to_s
      else
        # special handling for single characters
        case s
        when '9'
          ':'
        when 'Z'
          '['
        when 'z'
          '{'
        else
          s.succ
        end
      end
    end

    less_than = ->(a, b) do
      if treat_as_int_succ
        a.to_i < b.to_i
      else
        a < b
      end
    end

    s = self
    while less_than.(s, other_string)
      yield s
      s = special_succ.(s)
      return self if s.size > other_string.size
    end

    yield s unless exclusive

    self
  end
end
