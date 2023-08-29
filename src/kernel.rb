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

  # NATFIXME: Implement Warning class, check class for severity level
  def warn(*warnings, **)
    warnings.each { |warning| $stderr.puts(warning) }
    nil
  end

  class SprintfFormatter
    def initialize(format_string, arguments)
      @format_string = format_string
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

      if $DEBUG && arguments.any? && !@positional_argument_used
        raise ArgumentError, "too many arguments for format string"
      end

      result
    end

    private

    def format_field(token)
      arg = if token.arg_name
              get_named_argument(token.arg_name)
            elsif token.arg_position
              get_positional_argument(token.arg_position)
            else
              next_argument
            end

      if token.flags.include?(:width_given_as_arg)
        token.width = arg
        arg = next_argument
      end

      val = case token.datum
            when nil
              if token.arg_name
                # %{foo} doesn't require a specifier
                arg.to_s
              else
                raise ArgumentError, 'malformed format string'
              end
            when 'b'
              format_binary(token, arg)
            when 'B'
              format_binary(token, arg).upcase
            when 'c'
              format_char(token, arg)
            when 'd', 'u', 'i'
              format_integer(token, arg)
            when 'e', 'E', 'f', 'g', 'G'
              token.precision ||= 6
              format_float(token, arg)
            when 'o'
              format_octal(token, arg)
            when 'p'
              arg.inspect
            when 's'
              arg.to_s
            when 'x'
              format_hex(token, arg)
            when 'X'
              format_hex(token, arg).upcase
            else
              raise ArgumentError, "malformed format string - %#{token.datum}"
            end

      pad_value(token, val)
    end

    def pad_value(token, val)
      pad_char = token.flags.include?(:zero_padded) ? '0' : ' '

      while token.width && val.size < token.width
        val = pad_char + val
      end

      if token.width && token.width < 0
        val << pad_char while val.size < token.width.abs
      end

      val
    end

    def build_numeric_value_with_padding(token:, sign:, value:, prefix: nil, dotdot_sign: nil)
      width = token.width
      return "#{sign}#{prefix}#{dotdot_sign}#{value}" unless width

      sign_size = sign&.size || 0
      prefix_size = prefix&.size || 0
      dotdot_sign_size = dotdot_sign&.size || 0

      pad_char = token.flags.include?(:zero_padded) && !width.negative? ? '0' : ' '
      needed = width.abs - sign_size - prefix_size - dotdot_sign_size - value.size
      padding = pad_char * [needed, 0].max

      if width.negative?
        "#{sign}#{prefix}#{value}#{padding}"
      elsif pad_char == '0'
        "#{sign}#{prefix}#{padding}#{value}"
      else
        "#{padding}#{prefix}#{sign}#{value}"
      end
    end

    def format_binary(token, arg)
      i = convert_int(arg)

      sign = ''

      if i.negative?
        if (token.flags & [:space, :plus]).any?
          sign = '-'
          value = i.abs.to_s(2)
        else
          dotdot_sign = '..'
          width = token.precision - 2 if token.precision
          value = twos_complement(arg, 2, width)
        end
      else
        if token.flags.include?(:plus)
          sign = '+'
        elsif token.flags.include?(:space)
          sign = ' '
        end
        value = i.abs.to_s(2)
      end

      if token.flags.include?(:alternate_format) && value != '0'
        prefix = '0b'
      end

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

    def twos_complement(num, base, width)
      # See this comment in the Ruby source for how this should work:
      # https://github.com/ruby/ruby/blob/3151d7876fac408ad7060b317ae7798263870daa/sprintf.c#L662-L670
      needed_bits = num.abs.to_s(2).size + 1
      bits = [width.to_i, needed_bits].max
      first_digit = (base - 1).to_s
      bad_start = first_digit + first_digit
      loop do
        result = (2**bits - num.abs).to_s(base)
        bits += 1
        if result.start_with?(first_digit) && !result.start_with?(bad_start)
          return result 
        end
        raise 'something went wrong' if bits > 128 # arbitrarily chosen upper sanity limit
      end
    end

    def format_char(token, arg)
      if arg.is_a?(Integer)
        arg.chr
      elsif arg.respond_to?(:to_ary)
        arg.to_ary.first
      elsif arg.respond_to?(:to_int)
        arg.to_int.chr
      elsif arg.respond_to?(:to_str)
        s = arg.to_str
        if s.is_a?(String)
          raise ArgumentError, '%c requires a character' if s.size > 1
          s
        else
          raise TypeError, "can't convert Object to String (#{arg.class.name}#to_str gives #{s.class.name})"
        end
      else
        raise TypeError, "no implicit conversion of #{arg.class.name} into Integer"
      end
    end

    def format_integer(token, arg)
      i = convert_int(arg)

      sign = ''

      if i.negative?
        sign = '-'
        value = i.abs.to_s
      else
        if token.flags.include?(:plus)
          sign = '+'
        elsif token.flags.include?(:space)
          sign = ' '
        end
        value = i.abs.to_s
      end

      if token.precision
        value = ('0' * ([token.precision - value.size, 0].max)) + value
      end

      build_numeric_value_with_padding(
        token: token,
        sign: sign,
        value: value
      )
    end

    def format_float(token, arg)
      f = if arg.is_a?(Float)
        arg.to_s
      elsif arg.respond_to?(:to_ary)
        arg.to_ary.first.to_s
      else
        Float(arg).to_s
      end
      f = apply_number_flags(f, token.flags)
      f << '.0' unless f.index('.')
      f << '0' until f.split('.').last.size >= token.precision
      f
    end

    def format_octal(token, arg)
      i = convert_int(arg)

      sign = ''

      if i.negative?
        if (token.flags & [:space, :plus]).any?
          sign = '-'
          value = i.abs.to_s(8)
        else
          dotdot_sign = '..'
          width = token.precision - 2 if token.precision
          value = twos_complement(arg, 8, width)
        end
      else
        if token.flags.include?(:plus)
          sign = '+'
        elsif token.flags.include?(:space)
          sign = ' '
        end
        value = i.abs.to_s(8)
      end

      if token.flags.include?(:alternate_format) && value != '0'
        prefix = '0'
      end

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

    def format_hex(token, arg)
      x = convert_int(arg).to_s(16)
      x = "0x#{x}" if token.flags.include?(:alternate_format) && x != '0'
      apply_number_flags(x, token.flags)
    end

    def apply_number_flags(num, flags)
      num = "+#{num}" if flags.include?(:plus) && !num.start_with?('-') && !num.start_with?('..')
      num = " #{num}" if flags.include?(:space) && !flags.include?(:plus) && !num.start_with?('-')
      num
    end

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
      elsif i.respond_to?(:to_ary)
        i = i.to_ary.first
        raise ArgumentError unless i.is_a?(Integer)
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
        literal: {
          default: :literal,
          on_percent: :field_pending,
        },
        literal_percent: {
          default: :literal,
        },
        field_flag: {
          return: :field_pending,
        },
        field_named_argument_angled: {
          on_greater_than: :field_pending,
          default: :field_named_argument_angled,
        },
        field_named_argument_curly: {
          on_right_curly_brace: :field_named_argument_curly_end,
          default: :field_named_argument_curly,
        },
        field_named_argument_curly_end: {
          default: :literal,
          on_percent: :field_pending,
        },
        field_pending: {
          on_alpha: :field_end,
          on_asterisk: :field_flag,
          on_minus: :field_width_minus,
          on_less_than: :field_named_argument_angled,
          on_left_curly_brace: :field_named_argument_curly,
          on_newline: :literal_percent,
          on_null_byte: :literal_percent,
          on_number: :field_width,
          on_percent: :literal,
          on_period: :field_precision_period,
          on_plus: :field_flag,
          on_pound: :field_flag,
          on_space: :field_flag,
          on_zero: :field_flag,
        },
        field_end: {
          default: :literal,
          on_percent: :field_pending,
        },
        field_precision: {
          on_alpha: :field_end,
          on_number: :field_precision,
          on_zero: :field_precision,
        },
        field_precision_period: {
          on_number: :field_precision,
        },
        field_width: {
          on_dollar: :positional_argument,
          on_number: :field_width,
          on_zero: :field_width,
          return: :field_pending,
        },
        field_width_minus: {
          on_number: :field_width,
          on_zero: :field_width,
        },
        positional_argument: {
          return: :field_pending,
        }
      }.freeze

      COMPLETE_STATES = %i[
        field_end
        literal
        literal_percent
      ].freeze

      class Token
        def initialize(type:, datum:, flags: [], width: nil, precision: nil, arg_position: nil, arg_name: nil)
          @type = type
          @datum = datum
          @flags = flags
          @width = width
          @precision = precision
          @arg_position = arg_position
          @arg_name = arg_name
        end

        attr_accessor :type, :datum, :flags, :width, :precision, :arg_position, :arg_name
      end

      def tokens
        state = :literal
        width = nil
        width_negative = false
        precision = nil
        arg_position = nil
        arg_name = nil
        flags = []
        tokens = []

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
          when :field_pending, :field_precision_period
            :noop
          when :field_flag
            flags << case char
                    when '#'
                      :alternate_format
                    when ' '
                      :space
                    when '+'
                      :plus
                    when '0'
                      :zero_padded
                    when '*'
                      raise ArgumentError, 'width given twice' if width || flags.include?(:width_given_as_arg)
                      :width_given_as_arg
                    else
                      raise ArgumentError, "unknown flag: #{char.inspect}"
                    end
          when :field_width
            width = (width || 0) * 10 + char.to_i
          when :field_width_from_arg
            width = :from_arg
          when :field_width_minus
            width_negative = true
          when :field_precision
            precision = (precision || 0) * 10 + char.to_i
          when :field_named_argument_angled, :field_named_argument_curly
            if arg_name
              arg_name << char
            else
              arg_name = ''
            end
          when :field_named_argument_curly_end
            tokens << Token.new(
              type: :field,
              datum: nil,
              arg_name: arg_name,
            )
            arg_name = nil
          when :field_end
            tokens << Token.new(
              type: :field,
              datum: char,
              flags: flags.dup,
              width: width_negative ? -width : width,
              precision: precision,
              arg_position: arg_position,
              arg_name: arg_name,
            )
            width = nil
            width_negative = false
            precision = nil
            arg_position = nil
            arg_name = nil
          when :positional_argument
            new_arg_position = width
            width = nil
            if arg_position
              raise ArgumentError, "value given twice - #{new_arg_position}$"
            end
            arg_position = new_arg_position
          else
            raise ArgumentError, "unknown state: #{state.inspect}"
          end
        end

        # An incomplete field with no type and having a positional argument
        # produces a literal '%'.
        if state == :positional_argument
          tokens << Token.new(type: :literal, datum: '%')
          state = :field_end
        end

        unless COMPLETE_STATES.include?(state)
          raise ArgumentError, 'malformed format string'
        end

        tokens
      end

      private

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
