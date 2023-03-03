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

    # Get non-exclusive flags that occur after a leading '%'
    # but before the width field
    get_flags = ->(format_chars, index) {
      flags = []
      while [' ', '#', '-', '+', '0'].include?(format_chars[index])
        case format_chars[index]
        when ' ' then flags << :space
        when '#' then flags << :alter
        when "0" then flags << :zero
        when "+" then flags << :plus
        when "-" then flags << :left
        else raise NotImplementedError, "invalid branch"
        end
        index += 1
      end
      [flags, index]
    }

    # Lambda to capture 'n$' Flag.
    # If a numeric value is found that does not end with '$'
    # the index will move backward so a later proc may capture it
    get_position_flag = -> (format_chars, index) {
      original_index = index
      position = nil
      while ('0'..'9').cover?(format_chars[index])
        index += 1
      end
      if format_chars[index] == "$"
        position = format_chars[original_index ... index].join.to_i - 1
        index += 1
      else
        index = original_index
      end
      [position, index]
    }

    # Lambda to capture width integer, else returns nil
    get_width = -> (format_chars, index) {
      original_index = index
      number = nil
      while ('0'..'9').cover?(format_chars[index])
        index += 1
      end
      if original_index != index
        number = format_chars[original_index ... index].join.to_i
      end
      [number, index]
    }
    
    # Lambda to capture precision integer, else returns nil
    get_precision = -> (format_chars, index) {
      precision = nil
      if format_chars[index] == '.'
        index += 1
        precision, index = get_width(format_chars, index)
      end
      [precision, index]
    }
        
    append = ->(format_char, flags: [], arg_index: nil) {
      get_arg = -> {
        if arg_index
          positional_args_used = true
          positional_args[arg_index]
        else
          raise ArgumentError, "too few arguments" if args.empty?
          args.shift
        end
      }

      case format_char
      # Integer Type Specifiers
      when 'b', 'B', 'd', 'i', 'u', 'o', 'x', 'X'
        arg = get_arg.()
        if arg > 0
          if flags.include?(:plus)
            result << "+"
          elsif flags.include?(:space)
            result << " "
          end
        end
        case format_char
        when 'b', 'B'
          result << ("0" + format_char) if flags.include?(:alter)
          result << arg.to_s(2)
        when 'd', 'i', 'u'
          result << arg.to_s
        when 'o'
          result << '0' if flags.include?(:alter)
          result << arg.to_s(8)
        when 'x'
          result << ("0" + format_char) if arg != 0 && flags.include?(:alter)
          result << arg.to_s(16)
        when 'X'
          result << ("0" + format_char) if arg != 0 && flags.include?(:alter)
          result << arg.to_s(16).upcase
        else raise NotImplementedError, "invalid branch"
        end
      
      # Other Type Specifiers
      when 'c' # Character
        arg = get_arg.()
        if arg.is_a? Integer
          if arg < 0 && arg > 256
            raise NotImplementedError, "Cannot convert value #{arg} to codepoint"
          end
          result << arg.chr
        else
          if !arg.is_a?(String) && arg.respond_to?(:to_str)
            arg = arg.to_str
          end
          raise TypeError, "expected a string, got #{arg.inspect}" unless arg.is_a?(String)
          raise ArgumentError, "invalid character #{arg}" if arg.size > 1
          result << arg.to_s
        end
      when 'p'
        result << get_arg.().inspect
      when 's'
        result << get_arg.().to_s
      when '%'
        result << '%'
      when "\n"
        result << "%\n"
      when "\0"
        result << "%\0"
      when ' '
        raise ArgumentError, 'invalid format character - %'
      when nil
        if arg_index
          result << '%'
        else
          raise ArgumentError, 'incomplete format specifier; use %% (double %) instead'
        end
      else
        raise ArgumentError, "malformed format string - %#{format_char}"
      end
    }

    while index < format.size
      c = format[index]
      case c
      when '%'
        index += 1
        flaglist, index = get_flags.(format, index)
        position, index = get_position_flag.(format, index)
        width, index = get_width.(format, index)
        precision, index = get_precision.(format, index)

        f = format[index]
        # debug
        p fmtstr: self
        p flags: flaglist, pos: position, wid: width, prec: precision, f: f
        
        append.(f, flags: flaglist, arg_index: position)
#        
#        case f
#        when '0'..'9'
#          d = f
#          position = 0
#          begin
#            position = position * 10 + d.to_i
#            index += 1
#            d = format[index]
#          end while ('0'..'9').cover?(d)
#          case d
#          when '$' # position
#            index += 1
#            if (f = format[index])
#              append.(f, flags: flaglist, arg_index: position - 1)
#            else
#              result << '%'
#            end
#          else
#            raise NotImplementedError, "todo, last received format-char was #{d.inspect} at index #{index}"
#          end
#        else
#          append.(f, flags: flaglist)
#        end
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
