class String
  alias replace initialize_copy
  alias slice []

  def %(args)
    args = Array(args)
    index = 0
    format = chars
    result = []

    while index < format.size
      c = format[index]
      case c
      when '%'
        index += 1
        c2 = format[index]
        case c2
        when 'd'
          result << args.shift.to_s
        when 'b'
          result << args.shift.to_s(2)
        when 'x'
          result << args.shift.to_s(16)
        when 's'
          result << args.shift.to_s
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
          raise ArgumentError, "malformed format string - %#{c2}"
        end
      else
        result << c
      end
      index += 1
    end

    if $DEBUG && args.any?
      raise ArgumentError, 'too many arguments for format string'
    end

    result.join
  end
end
