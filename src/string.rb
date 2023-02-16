class String
  alias replace initialize_copy
  alias slice []

  def %(args)
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
        else
          raise "I don't yet know how to handle format '#{c2}'"
        end
      else
        result << c
      end
      index += 1
    end

    result.join
  end
end
