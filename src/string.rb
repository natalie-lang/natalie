class String
  alias replace initialize_copy
  alias slice []

  def %(args)
    positional_args = if args.respond_to?(:to_ary)
                        if (ary = args.to_ary).nil?
                          [args]
                        elsif ary.is_a?(Array)
                          ary
                        else
                          raise TypeError, "can't convert #{args.class.name} to Array (#{args.class.name}#to_ary gives #{ary.class.name})"
                        end
                      else
                        [args]
                      end
    Kernel::sprintf(self, *positional_args)
  end

  def to_c
    real = +''
    imaginary = +''
    state = :start
    chars.each do |c|
      case state
      when :start
        if c >= '0' && c <= '9'
          real << c
          state = :real_integer
        elsif c == '+' || c == '-'
          real << c if c == '-'
          state = :real_integer
        elsif c == 'i'
          imaginary = '1'
          break
        end
      when :real_integer
        if c >= '0' && c <= '9'
          real << c
        elsif c == '.'
          real << '.'
          state = :real_fractional
        elsif c == '+' || c == '-'
          imaginary << c if c == '-'
          state = :imaginary_integer
        elsif c == 'i'
          imaginary = real == '-' ? '-1' : real
          real = '0'
          break
        else
          break
        end
      when :real_fractional
        if c >= '0' && c <= '9'
          real << c
        elsif c == '+' || c == '-'
          imaginary << c if c == '-'
          state = :imaginary_integer
        elsif c == 'i'
          imaginary = real
          real = '0'
        else
          break
        end
      when :imaginary_integer
        if c >= '0' && c <= '9'
          imaginary << c
        elsif c == '.'
          imaginary << c
          state = :imaginary_fractional
        elsif c == 'i'
          imaginary << '1' if imaginary.empty? || imaginary == '+' || imaginary == '-'
          break
        else
          imaginary = ''
          break
        end
      when :imaginary_fractional
        if c >= '0' && c <= '9'
          imaginary << c
        elsif c == 'i'
          break
        else
          imaginary = ''
          break
        end
      else
        raise "not yet implemented: #{state}"
      end
    end
    real = '0' if real.empty? || real == '-' || real == '+'
    imaginary = '0' if imaginary.empty? || imaginary == '-' || imaginary == '+'
    real = real.include?('.') ? Float(real) : Integer(real)
    imaginary = imaginary.include?('.') ? Float(imaginary) : Integer(imaginary)
    Complex(real, imaginary)
  end
end
