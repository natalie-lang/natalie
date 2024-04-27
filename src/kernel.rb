require 'natalie/inline'

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
        the_proc = yielder.to_proc || ->(*i) { yielder.yield(*i) }
        send(method, *args, **kwargs, &the_proc)
      end
    if block_given?
      enum.instance_variable_set(:@size_block, block)
      def enum.size
        @size_block.call
      end
    end
    enum
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

  def warn(*msgs, category: nil, uplevel: nil)
    if !category.nil? && !category.is_a?(Symbol)
      category = category.to_sym if category.respond_to?(:to_sym)
      raise TypeError, "no implicit conversion of #{category.class} into Symbol" unless category.is_a?(Symbol)
    end
    location = nil
    unless uplevel.nil?
      uplevel = uplevel.to_int if !uplevel.is_a?(Integer) && uplevel.respond_to?(:to_int)
      raise TypeError, "no implicit conversion of #{uplevel.class} into Integer" unless uplevel.is_a?(Integer)
      raise ArgumentError, "negative level (#{uplevel})" if uplevel&.negative?
      backtrace = caller_locations(uplevel + 1, 1)&.first
      location = "warning: "
      location.prepend("#{backtrace.path}:#{backtrace.lineno}: ") unless backtrace.nil?
    end


    msgs = msgs[0] if msgs.size == 1 && msgs[0].is_a?(Array)
    msgs.each do |message|
      message = message.to_s
      message = message + "\n" unless message.end_with?("\n")
      message = location + message unless location.nil?
      Warning.warn(message, category: category)
    end
    nil
  end
  module_function :warn

  class SprintfFormatter
    def initialize(format_string, arguments)
      @format_string = format_string.to_str
      @arguments = arguments
      @arguments_index = 0
      @positional_argument_used = nil
      @unnumbered_argument_used = nil
    end

    attr_reader \
      :format_string,
      :arguments,
      :arguments_index

    def format
      tokens = Parser.new(format_string).tokens

      result = tokens.map do |token|
        case token.type
        when :literal
          token.datum
        when :field
          format_field(token)
        else
          raise "unknown token: #{token.inspect}"
        end
      end.join

      begin
        result = result.encode(format_string.encoding)
      rescue ArgumentError, Encoding::UndefinedConversionError
        # we tried
      end

      if $DEBUG && arguments.any? && !@positional_argument_used
        raise ArgumentError, "too many arguments for format string"
      end

      result
    end

    private

    def format_field(token)
      token.flags.each do |flag|
        case flag
        when :width_given_as_arg
          if token.width_arg_position
            token.width = int_from_arg(get_positional_argument(token.width_arg_position))
          else
            token.width = int_from_arg(next_argument)
          end
          token.width *= -1 if token.flags.include?(:width_negative) && !token.width.negative?
        when :precision_given_as_arg
          if token.precision_arg_position
            token.precision = int_from_arg(get_positional_argument(token.precision_arg_position))
          else
            token.precision = int_from_arg(next_argument)
          end
        end
      end

      val = if token.value_arg_name
              get_named_argument(token.value_arg_name)
            elsif token.value_arg_position
              get_positional_argument(token.value_arg_position)
            else
              next_argument
            end

      val = case token.datum
            when nil
              if token.value_arg_name
                # %{foo} doesn't require a specifier
                s = format_str(token, val)
              else
                raise ArgumentError, 'malformed format string'
              end
            when 'a', 'A'
              format_float_as_hex(token, float_from_arg(val))
            when 'b'
              format_binary(token, val)
            when 'B'
              format_binary(token, val).upcase
            when 'c'
              format_char(token, val)
            when 'd', 'u', 'i'
              format_integer(token, val)
            when 'e'
              format_float_with_e_notation(token, float_from_arg(val), e: 'e')
            when 'E'
              format_float_with_e_notation(token, float_from_arg(val), e: 'E')
            when 'f'
              format_float(token, float_from_arg(val))
            when 'g'
              format_float_g(token, float_from_arg(val), e: 'e')
            when 'G'
              format_float_g(token, float_from_arg(val), e: 'E')
            when 'o'
              format_octal(token, val)
            when 'p'
              format_inspect(token, val)
            when 's'
              format_str(token, val)
            when 'x'
              format_hex(token, val)
            when 'X'
              format_hex(token, val).upcase
            else
              raise ArgumentError, "malformed format string - %#{token.datum}"
            end

      pad_value(token, val)
    end

    def float_from_arg(arg)
      f = if arg.is_a?(Float)
            arg
          else
            Float(arg)
          end
      unless f.is_a?(Float)
        raise TypeError, "no implicit conversion of #{arg.class.name} into Float"
      end
      f
    end

    def int_from_arg(arg)
      int = if arg.is_a?(Integer)
              arg
            elsif arg.respond_to?(:to_int)
              arg.to_int
            end
      unless int.is_a?(Integer)
        raise TypeError, "no implicit conversion of #{arg.class.name} into Integer"
      end
      int
    end

    def pad_value(token, val)
      pad_char = token.flags.include?(:zero) ? '0' : ' '

      while token.width && val.size < token.width
        val = pad_char + val
      end

      if token.width && token.width < 0
        val << pad_char while val.size < token.width.abs
      end

      val
    end

    def format_str(token, arg)
      s = arg.to_s
      if token.precision && s.size > token.precision
        s = s[0...token.precision]
      end
      if token.width && s.size < token.width
        s = (' ' * (token.width - s.size)) + s
      end
      s
    end

    def format_char(token, arg)
      if arg.nil?
        raise TypeError, 'no implicit conversion from nil to integer'
      elsif arg.is_a?(Integer)
        arg.chr(format_string.encoding)
      elsif arg.respond_to?(:to_int)
        i = arg.to_int
        unless i.is_a?(Integer)
          raise TypeError, "can't convert Object to Integer (#{arg.class.name}#to_int gives #{i.class.name})"
        end
        i.chr(format_string.encoding)
      elsif arg.respond_to?(:to_str)
        s = arg.to_str
        unless s.is_a?(String)
          raise TypeError, "can't convert Object to String (#{arg.class.name}#to_str gives #{s.class.name})"
        end
        s[0]
      else
        raise TypeError, "no implicit conversion of #{arg.class.name} into Integer"
      end
    rescue NoMethodError
      raise unless Kernel.instance_method(:instance_of?).bind(arg).call(BasicObject)
      begin
        i = arg.to_int
        unless i.is_a?(Integer)
          raise TypeError, "can't convert BasicObject to Integer"
        end
        i.chr(format_string.encoding)
      rescue NoMethodError
        begin
          s = arg.to_str
          unless s.is_a?(String)
            raise TypeError, "can't convert BasicObject to String"
          end
          s[0]
        rescue NoMethodError
          raise TypeError, "no implicit conversion of BasicObject into Integer"
        end
      end
    end

    def format_float(token, f)
      token.precision ||= 6
      sprintf(token.c_printf_format, f).sub(/inf/i, 'Inf').sub(/-?nan/i, 'NaN')
    end

    def format_float_g(token, f, e: 'e')
      token.precision ||= 6
      sprintf(token.c_printf_format, f).sub(/inf/i, 'Inf').sub(/-?nan/i, 'NaN')
    end

    def format_float_with_e_notation(token, f, e: 'e')
      token.precision ||= 6
      sprintf(token.c_printf_format, f).sub(/inf/i, 'Inf').sub(/-?nan/i, 'NaN')
    end

    def format_float_as_hex(token, f)
      sprintf(token.c_printf_format, f).sub(/inf/i, 'Inf').sub(/-?nan/i, 'NaN')
    end

    def format_integer(token, arg)
      format_number(token: token, arg: arg, base: 10, bits_per_char: 4, prefix: '')
    end

    def format_binary(token, arg)
      format_number(token: token, arg: arg, base: 2, bits_per_char: 1, prefix: '0b')
    end

    def format_octal(token, arg)
      format_number(token: token, arg: arg, base: 8, bits_per_char: 3, prefix: '0')
    end

    def format_hex(token, arg)
      format_number(token: token, arg: arg, base: 16, bits_per_char: 4, prefix: '0x')
    end

    def format_number(token:, arg:, base:, bits_per_char:, prefix:)
      i = convert_int(arg)

      sign = ''

      if i.negative?
        if (token.flags & [:space, :plus]).any? || base == 10
          sign = '-'
          value = i.abs.to_s(base)
        else
          dotdot_sign = '..'
          width = if token.precision
                    (token.precision.to_i - 2) * bits_per_char
                  elsif token.width && token.flags.include?(:zero)
                    (token.width.to_i - 2) * bits_per_char
                  else
                    0
                  end
          value = twos_complement(arg, base, [width, 0].max)
        end
      else
        if token.flags.include?(:plus)
          sign = '+'
        elsif token.flags.include?(:space)
          sign = ' '
        end
        value = i.abs.to_s(base)
      end

      prefix = '' unless token.flags.include?(:alternate_format)

      # '%#x' % 0 should not produce '0x0'
      prefix = '' if value == '0'

      # '%o' % -10 should not produce '0..766'
      prefix = '' if prefix == '0' && i.negative?

      if token.precision
        needed = token.precision - value.size - (dotdot_sign&.size || 0)
        value = ('0' * ([needed, 0].max)) + value
      end

      build_numeric_value_with_padding(
        token: token,
        sign: sign,
        value: value,
        prefix: prefix,
        dotdot_sign: dotdot_sign
      )
    end

    def format_inspect(token, val)
      str = val.inspect

      if token.precision && token.precision < str.size
        str = str[0...token.precision]
      end

      str
    end

    def build_numeric_value_with_padding(token:, sign:, value:, prefix: nil, dotdot_sign: nil)
      width = token.width
      return "#{sign}#{prefix}#{dotdot_sign}#{value}" unless width

      sign_size = sign&.size || 0
      prefix_size = prefix&.size || 0
      dotdot_sign_size = dotdot_sign&.size || 0

      pad_char = token.flags.include?(:zero) && !width.negative? ? '0' : ' '
      needed = width.abs - sign_size - prefix_size - dotdot_sign_size - value.size
      padding = pad_char * [needed, 0].max

      if width.negative?
        "#{sign}#{prefix}#{dotdot_sign}#{value}#{padding}"
      elsif pad_char == '0'
        "#{sign}#{prefix}#{padding}#{dotdot_sign}#{value}"
      else
        "#{padding}#{prefix}#{sign}#{dotdot_sign}#{value}"
      end
    end

    def twos_complement(num, base, width)
      # See this comment in the Ruby source for how this should work:
      # https://github.com/ruby/ruby/blob/3151d7876fac408ad7060b317ae7798263870daa/sprintf.c#L662-L670
      needed_bits = num.abs.to_s(2).size + 1
      bits = [width.to_i, needed_bits].max
      first_digit = (base - 1).to_s(base)
      result = nil
      loop do
        result = (2**bits - num.abs).to_s(base)
        bits += 1
        if result.start_with?(first_digit)
          break
        end
        raise 'something went wrong' if bits > 128 # arbitrarily chosen upper sanity limit
      end
      if result == first_digit + first_digit
        # ..11 can be represented as just ..1
        first_digit
      else
        result
      end
    end

    __define_method__ :sprintf, [:format, :val], <<-END
      assert(format->is_string());
      assert(val->is_float());
      char buf[100];
      auto fmt = format->as_string()->c_str();
      auto dbl = val->as_float()->to_double();
      if (snprintf(buf, 100, fmt, dbl) > 0) {
          if (isnan(dbl) && strcasestr(buf, "-nan")) {
              // dumb hack to fix -NAN on some systems
              dbl *= -1;
              if (snprintf(buf, 100, fmt, dbl) > 0)
                  return new StringObject { buf };
          } else {
              return new StringObject { buf };
          }
      }
      env->raise("ArgumentError", "could not format value");
    END

    def next_argument
      if arguments_index >= arguments.size
        raise ArgumentError, 'too few arguments'
      end
      if @positional_argument_used
        raise ArgumentError, "unnumbered(#{arguments_index}) mixed with numbered"
      end
      arg = arguments[arguments_index]
      @unnumbered_argument_used = arguments_index
      @arguments_index += 1
      arg
    end

    def get_named_argument(name)
      if arguments.size == 1 && arguments.first.is_a?(Hash)
        arguments.first.fetch(name.to_sym)
      else
        raise ArgumentError, 'one hash required'
      end
    end

    def get_positional_argument(position)
      if position > arguments.size
        raise ArgumentError, 'too few arguments'
      end
      if @unnumbered_argument_used
        raise ArgumentError, "numbered(#{position}) after unnumbered(#{@unnumbered_argument_used})"
      end
      @positional_argument_used = position
      arguments[position - 1]
    end

    def convert_int(i)
      if i.is_a?(Integer)
        i
      else
        Integer(i)
      end
    end

    class Parser
      def initialize(format_string)
        @format_string = format_string
        @index = 0
        @chars = format_string.chars
      end

      attr_reader :format_string, :index, :chars

      STATES_AND_TRANSITIONS = {
        field: {
          on_alpha: :field_end,
          on_asterisk: :width_from_arg,
          on_left_curly_brace: :named_argument_curly,
          on_less_than: :named_argument_angled,
          on_minus: :width_minus,
          on_newline: :literal_percent,
          on_null_byte: :literal_percent,
          on_number: :width_or_positional_arg,
          on_percent: :literal,
          on_period: :precision_period,
          on_plus: :flag,
          on_pound: :flag,
          on_space: :flag,
          on_zero: :flag,
        },
        field_end: {
          default: :literal,
          on_percent: :field,
        },
        flag: {
          return: :field,
        },
        literal: {
          default: :literal,
          on_percent: :field,
        },
        literal_percent: {
          default: :literal,
        },
        named_argument_angled: {
          on_greater_than: :field,
          default: :named_argument_angled,
        },
        named_argument_curly: {
          on_right_curly_brace: :named_argument_curly_end,
          default: :named_argument_curly,
        },
        named_argument_curly_end: {
          default: :literal,
          on_percent: :field,
        },
        positional_argument_end: {
          return: :field,
        },
        precision: {
          on_alpha: :field_end,
          on_left_curly_brace: :named_argument_curly,
          on_less_than: :named_argument_angled,
          on_number: :precision,
          on_zero: :precision,
        },
        precision_from_arg: {
          on_number: :precision_from_positional_arg,
          return: :field,
        },
        precision_from_positional_arg: {
          on_dollar: :precision_from_positional_arg_end,
          on_number: :precision_from_positional_arg,
          on_zero: :precision_from_positional_arg,
          return: :field,
        },
        precision_from_positional_arg_end: {
          return: :field,
        },
        precision_period: {
          on_number: :precision,
          on_zero: :precision,
          on_asterisk: :precision_from_arg,
        },
        width_from_arg: {
          on_number: :width_from_positional_arg,
          return: :field,
        },
        width_from_positional_arg: {
          on_dollar: :width_from_positional_arg_end,
          on_number: :width_from_positional_arg,
          on_zero: :width_from_positional_arg,
          return: :field,
        },
        width_from_positional_arg_end: {
          return: :field,
        },
        width_minus: {
          on_number: :width_or_positional_arg,
          on_zero: :width_or_positional_arg,
          on_asterisk: :width_from_arg,
        },
        width_or_positional_arg: {
          on_dollar: :positional_argument_end,
          on_number: :width_or_positional_arg,
          on_zero: :width_or_positional_arg,
          return: :field,
        },
      }.freeze

      COMPLETE_STATES = %i[
        field_end
        literal
        literal_percent
        named_argument_curly_end
      ].freeze

      class Token
        def initialize(type:, datum:, flags: [], width: nil, width_arg_position: nil, precision: nil, precision_arg_position: nil, value_arg_position: nil, value_arg_name: nil)
          @type = type
          @datum = datum
          @flags = flags
          @width = width
          @width_arg_position = width_arg_position
          @precision = precision
          @precision_arg_position = precision_arg_position
          @value_arg_position = value_arg_position
          @value_arg_name = value_arg_name
        end

        attr_accessor :type, :datum, :flags,
          :width, :width_arg_position,
          :precision, :precision_arg_position,
          :value_arg_position, :value_arg_name

        def c_printf_format
          flag_chars = {
            alternate_format: '#',
            space: ' ',
            plus: '+',
            zero: '0',
          }.select { |k| flags.include?(k) }.values.join
          prec = ".#{precision}" if precision
          "%#{flag_chars}#{width}#{prec}#{datum}"
        end
      end

      def tokens
        state = :literal
        tokens = []
        reset_token_args

        while index < chars.size
          char = current_char
          transition = case char
                      when '%'
                        :on_percent
                      when "\n"
                        :on_newline
                      when "\0"
                        :on_null_byte
                      when '.'
                        :on_period
                      when '#'
                        :on_pound
                      when '+'
                        :on_plus
                      when '-'
                        :on_minus
                      when '*'
                        :on_asterisk
                      when '0'
                        :on_zero
                      when ' '
                        :on_space
                      when '$'
                        :on_dollar
                      when '<'
                        :on_less_than
                      when '>'
                        :on_greater_than
                      when '{'
                        :on_left_curly_brace
                      when '}'
                        :on_right_curly_brace
                      when '1'..'9'
                        :on_number
                      when 'a'..'z', 'A'..'Z'
                        :on_alpha
                      end

          new_state = STATES_AND_TRANSITIONS.dig(state, transition) ||
            STATES_AND_TRANSITIONS.dig(state, :default)

          if !new_state && (return_state = STATES_AND_TRANSITIONS.dig(state, :return))
            # :return is a special transition that consumes no characters
            new_state = return_state
          end

          #puts "#{state.inspect}, given #{char.inspect}, " \
              #"transition #{transition.inspect} to #{new_state.inspect}"

          unless new_state
            raise ArgumentError, "no transition from #{state.inspect} with char #{char.inspect}"
          end

          state = new_state
          next_char unless return_state
          return_state = nil

          case state
          when :literal
            tokens << Token.new(type: :literal, datum: char)
          when :literal_percent
            tokens << Token.new(type: :literal, datum: "%#{char}")
          when :field, :precision_period
            :noop
          when :flag
            flags << case char
                    when '#'
                      :alternate_format
                    when ' '
                      :space
                    when '+'
                      :plus
                    when '0'
                      :zero
                    else
                      raise ArgumentError, "unknown flag: #{char.inspect}"
                    end
          when :width_or_positional_arg
            @width_or_positional_arg = (@width_or_positional_arg || 0) * 10 + char.to_i
          when :width_minus
            flags << :width_negative
          when :width_from_arg
            raise ArgumentError, 'width given twice' if @width_or_positional_arg || flags.include?(:width_given_as_arg)
            flags << :width_given_as_arg
          when :width_from_positional_arg
            @width_arg_position = (@width_arg_position || 0) * 10 + char.to_i
          when :width_from_positional_arg_end
            :noop
          when :precision
            raise ArgumentError, 'precision given twice' if flags.include?(:precision_given_as_arg)
            @precision = (@precision || 0) * 10 + char.to_i
            raise ArgumentError, 'precision too big' if precision > 2**64
          when :precision_from_arg
            raise ArgumentError, 'precision given twice' if precision || flags.include?(:precision_given_as_arg)
            flags << :precision_given_as_arg
          when :precision_from_positional_arg
            @precision_arg_position = (@precision_arg_position || 0) * 10 + char.to_i
          when :precision_from_positional_arg_end
            :noop
          when :named_argument_angled, :named_argument_curly
            if @value_arg_name
              @value_arg_name << char
            else
              @value_arg_name = ''
            end
          when :named_argument_curly_end
            tokens << build_token(nil)
            reset_token_args
          when :field_end
            tokens << build_token(char)
            reset_token_args
          when :positional_argument_end
            new_arg_position = @width_or_positional_arg
            @width_or_positional_arg = nil
            if @value_arg_position
              raise ArgumentError, "value given twice - #{new_arg_position}$"
            end
            @value_arg_position = new_arg_position
          else
            raise ArgumentError, "unknown state: #{state.inspect}"
          end
        end

        # An incomplete field with no type and having a positional argument
        # produces a literal '%'.
        if state == :positional_argument_end
          tokens << Token.new(type: :literal, datum: '%')
          state = :field_end
        end

        unless COMPLETE_STATES.include?(state)
          raise ArgumentError, "malformed format string #{state}"
        end

        tokens
      end

      private

      attr_reader \
        :flags,
        :precision_arg_position,
        :precision,
        :value_arg_name,
        :value_arg_position,
        :width_arg_position,
        :width,
        :width_or_positional_arg

      def reset_token_args
        @flags = []
        @precision_arg_position = nil
        @precision = nil
        @value_arg_name = nil
        @value_arg_position = nil
        @width_arg_position = nil
        @width_or_positional_arg = nil
      end

      def build_token(datum)
        if width_or_positional_arg
          width = if flags.include?(:width_negative)
                    -width_or_positional_arg
                  else
                    width_or_positional_arg
                  end
        end

        Token.new(
          type: :field,
          datum: datum,
          flags: flags.dup,
          width: width,
          width_arg_position: width_arg_position,
          precision: precision,
          precision_arg_position: precision_arg_position,
          value_arg_position: value_arg_position,
          value_arg_name: value_arg_name,
        )
      end

      def current_char
        chars[index]
      end

      def next_char
        @index += 1
        chars[index]
      end
    end
  end

  def sprintf(format_string, *arguments)
    SprintfFormatter.new(format_string, arguments).format
  end

  # NATFIXME: the ... syntax doesnt appear to pass the block
  def open(filename, *a, **kw, &blk)
    if filename.respond_to?(:to_open)
      result = filename.to_open(*a, **kw)
      return blk.nil? ? result : blk.call(result)
    end
    if !filename.respond_to?(:to_path) && !filename.is_a?(String) && filename.respond_to?(:to_str)
      filename = filename.to_str
    end
    if filename.is_a?(String) && filename.to_str.start_with?('|')
      raise NotImplementedError, 'no support for pipe in Kernel#open'
    end
    File.open(filename, *a, **kw, &blk)
  end
  module_function :open

  def putc(char)
    $stdout.putc(char)
  end
  module_function :putc

  alias format sprintf
  module_function :format

  def printf(*args)
    if args[0].is_a?(String)
      print(sprintf(*args))
    else
      args[0].write(sprintf(*args[1..]))
    end
  end
  module_function :printf

  def system(...)
    Process.wait(spawn(...))
    $?.exitstatus.zero?
  rescue
    nil
  end
end
