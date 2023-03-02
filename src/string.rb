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
    args = positional_args.dup
    index = 0
    format = chars
    result = []
    positional_args_used = false

    append = ->(format, arg_index: nil) {
      get_arg = -> {
        if arg_index
          positional_args_used = true
          positional_args[arg_index]
        else
          raise ArgumentError, "too few arguments" if args.empty?
          args.shift
        end
      }

      case format
      when 'b'
        result << get_arg.().to_s(2)
      when 'd'
        result << get_arg.().to_s
      # Other Type Specifiers
      when 'c' # Character
        arg = get_arg.()
        if arg.is_a? String
          raise ArgumentError, "invalid character #{arg}" if arg.size > 1
          result << arg.to_s
        elsif arg.is_a? Integer
          if arg < 0 && arg > 256
            raise NotImplementedError, "Cannot convert to codepoint"
          end
          result << arg.chr
        end
      when 'p'
        result << get_arg.().inspect
      when 's'
        result << get_arg.().to_s
      when 'x'
        result << get_arg.().to_s(16)
      when '%'
        result << '%'
      when "\n"
        result << "%\n"
      when "\0"
        result << "%\0"
      when ' '
        raise ArgumentError, 'invalid format character - %'
      when nil
        raise ArgumentError, 'incomplete format specifier; use %% (double %) instead'
      else
        raise ArgumentError, "malformed format string - %#{format}"
      end
    }

    while index < format.size
      c = format[index]
      case c
      when '%'
        index += 1
        f = format[index]
        case f
        when '0'..'9'
          d = f
          position = 0
          begin
            position = position * 10 + d.to_i
            index += 1
            d = format[index]
          end while ('0'..'9').cover?(d)
          case d
          when '$' # position
            index += 1
            if (f = format[index])
              append.(f, arg_index: position - 1)
            else
              result << '%'
            end
          else
            raise 'todo'
          end
        else
          append.(f)
        end
      else
        result << c
      end
      index += 1
    end

    if $DEBUG && args.any? && !positional_args_used
      raise ArgumentError, 'too many arguments for format string'
    end

    result.join
  end
end
