module Kernel
  def <=>(other)
    0 if other.object_id == self.object_id || (!is_a?(Comparable) && self == other)
  end

  def then
    if block_given?
      return yield(self)
    end

    Enumerator.new(1) do |yielder|
      yielder.yield(self)
    end
  end
  alias yield_self then

  def enum_for(method = :each, *args, **kwargs, &block)
    enum =
      Enumerator.new do |yielder|
        the_proc = enum_for_build_the_proc(yielder)
        send(method, *args, **kwargs, &the_proc)
      end
    enum_for_size_block(enum, &block);
  end
  alias to_enum enum_for

  def initialize_dup(other)
    initialize_copy(other)
    self
  end
  private :initialize_dup

  def initialize_clone(other, freeze: nil)
    initialize_copy(other)
    self
  end
  private :initialize_clone

  def instance_of?(clazz)
    raise TypeError, 'class or module required' unless clazz.is_a?(Module)

    # We have to use this bind because #self might not respond to #class
    # This can be the case if a BasicObject gets #instance_of? defined via #define_method
    Kernel.instance_method(:class).bind(self).call == clazz
  end

  def rand(*args)
    Random::DEFAULT.rand(*args)
  end

  # NATFIXME: Implement Warning class, check class for severity level
  def warn(*warnings, **)
    warnings.each { |warning| $stderr.puts(warning) }
    nil
  end

  def sprintf(fmt, *positional_args)
    args = positional_args.dup
    index = 0
    format = fmt.chars
    result = []
    positional_args_used = nil
    default_precision = 6

    # Get non-exclusive flags that occur after a leading '%'
    # but before the width field
    get_flags = ->(format_chars, index) {
      flags = []
      while [' ', '#', '-', '+', '0', '*'].include?(format_chars[index])
        case format_chars[index]
        when ' ' then flags << :space
        when '#' then flags << :alter
        when "0" then flags << :zero
        when "+" then flags << :plus
        when "-" then flags << :left
        when "*" then
          raise ArgumentError, "cannot specify '*' multiple times" if flags.include?(:star)
          flags << :star
        else
          raise NotImplementedError, "invalid branch"
        end
        index += 1
      end
      [flags, index]
    }

    # Lambda to capture 'n$' position flag.
    # If a numeric value is found that does not end with '$' the index
    # will move backward so a later proc may capture it as a width
    get_position_flag = -> (format_chars, index) {
      original_index = index
      position = nil
      while ('0'..'9').cover?(format_chars[index])
        index += 1
      end
      if format_chars[index] == "$"
        chars = format_chars[original_index ... index].join
        raise ArgumentError, "malformed format string - %$" if chars == '0' || chars == ''
        position = chars.to_i - 1
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
        precision, index = get_width.(format_chars, index)
        # nil case gets zero to disambiguate case of not seeing the '.'
        precision ||= 0
      end
      [precision, index]
    }

    append = ->(format_char, flags: [], arg_index: nil, width: nil, precision: nil) {
      get_arg = -> {
        if arg_index
          if [nil, true].include?(positional_args_used)
            positional_args_used = true
          else
            raise ArgumentError, "unnumbered mixed with numbered"
          end
          positional_args[arg_index]
        else
          if [nil, false].include?(positional_args_used)
            positional_args_used = false
          else
            raise ArgumentError, "unnumbered mixed with numbered"
          end
          raise ArgumentError, "too few arguments" if args.empty?
          args.shift
        end
      }
      if flags.include?(:star) && width.nil?
        star_width = get_arg.()
        width = star_width.abs if star_width != 0
        flags << :left if star_width < 0
      end

      case format_char
      # Integer Type Specifiers
      when 'b', 'B', 'd', 'f', 'i', 'u', 'o', 'x', 'X'
        arg = get_arg.()
        localresult = ""
        if arg > 0
          if flags.include?(:plus)
            localresult << "+"
          elsif flags.include?(:space)
            localresult << " "
          end
        end
        case format_char
        when 'b', 'B'
          localresult << ("0" + format_char) if flags.include?(:alter)
          localresult << arg.to_s(2)
        when 'd', 'i', 'u'
          localresult << arg.to_s
        when 'f'
          s = arg.to_s
          precision ||= default_precision
          if (decimal = s.index('.'))
            decimal_digits = s.size - decimal - 1
          else
            s << '.0'
            decimal_digits = 1
          end
          while decimal_digits < precision
            s << '0'
            decimal_digits += 1
          end
          while decimal_digits > precision
            s.chop!
            decimal_digits -= 1
          end
          localresult << s
        when 'o'
          localresult << '0' if flags.include?(:alter)
          localresult << arg.to_s(8)
        when 'x'
          localresult << ("0" + format_char) if arg != 0 && flags.include?(:alter)
          localresult << arg.to_s(16)
        when 'X'
          localresult << ("0" + format_char) if arg != 0 && flags.include?(:alter)
          localresult << arg.to_s(16).upcase
        else
          raise NotImplementedError, "invalid branch"
        end
        # Perform padding
        if width && localresult.size < width
          pad_amount = (width - localresult.size)
          localresult = if flags.include?(:left)
                          localresult + (' ' * pad_amount)
                        elsif flags.include?(:zero)
                          ('0' * pad_amount) + localresult
                        else
                          (' ' * pad_amount) + localresult
                        end
        end
        result << localresult
      # Other Type Specifiers
      when 'c', 'p', 's'
        localresult = ""
        case format_char
        when 'c' # Character
          arg = get_arg.()
          if arg.is_a? Integer
            if arg < 0 && arg > 256
              raise NotImplementedError, "Cannot convert value #{arg} to codepoint"
            end
            localresult << arg.chr
          else
            if !arg.is_a?(String) && arg.respond_to?(:to_str)
              arg = arg.to_str
            end
            raise TypeError, "expected a string, got #{arg.inspect}" unless arg.is_a?(String)
            raise ArgumentError, "invalid character #{arg}" if arg.size > 1
            localresult << arg.to_s
          end
        when 'p'
          localresult << get_arg.().inspect
        when 's'
          localresult << get_arg.().to_s
        else
          raise NotImplementedError, "invalid branch"
        end
        # Perform padding
        if width && localresult.size < width
          pad_amount = (width - localresult.size)
          localresult = if flags.include?(:left)
                          localresult + (' ' * pad_amount)
                        else
                          (' ' * pad_amount) + localresult
                        end
        end
        result << localresult
      when '%', "\n", "\0"
        if flags.any? || width || precision || arg_index
          raise ArgumentError, "invalid #{format_char.inspect} after '%' with flags"
        end
        result << '%' if format_char != '%'
        result << format_char
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
        append.(f, flags: flaglist, arg_index: position, width: width, precision: precision)
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

  # NATFIXME: the ... syntax doesnt appear to pass the block
  def open(*a, **kw, &blk)
    File.open(*a, **kw, &blk)
  end

  alias format sprintf

  def printf(*args)
    if args[0].is_a?(String)
      print(sprintf(*args))
    else
      args[0].write(sprintf(*args[1..]))
    end
  end
end
