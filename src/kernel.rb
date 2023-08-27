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

  class SprintfParser
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
          tokens << Token.new(type: :str, datum: char)
        when :literal_percent
          tokens << Token.new(type: :str, datum: "%#{char}")
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
        tokens << Token.new(type: :str, datum: '%')
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

  def sprintf(format_string, *arguments)
    tokens = SprintfParser.new(format_string).tokens

    apply_number_flags = lambda do |num, flags|
      num = "+#{num}" if flags.include?(:plus) && !num.start_with?('-')
      num = " #{num}" if flags.include?(:space) && !flags.include?(:plus) && !num.start_with?('-')
      num
    end

    arguments_index = 0
    positional_argument_used = false
    unnumbered_argument_used = false
    next_argument = lambda do
      if arguments_index >= arguments.size
        raise ArgumentError, 'too few arguments'
      end
      if positional_argument_used
        raise ArgumentError, "unnumbered(#{arguments_index}) mixed with numbered"
      end
      arg = arguments[arguments_index]
      unnumbered_argument_used = arguments_index
      arguments_index += 1
      arg
    end

    get_named_argument = lambda do |name|
      if arguments.size == 1 && arguments.first.is_a?(Hash)
        arguments.first.fetch(name.to_sym)
      else
        raise ArgumentError, 'one hash required'
      end
    end

    get_positional_argument = lambda do |position|
      if position > arguments.size
        raise ArgumentError, 'too few arguments'
      end
      if unnumbered_argument_used
        raise ArgumentError, "numbered(#{position}) after unnumbered(#{unnumbered_argument_used})"
      end
      positional_argument_used = position
      arguments[position - 1]
    end

    convert_int = lambda do |i|
      if i.is_a?(Integer)
        i
      elsif i.respond_to?(:to_ary)
        i.to_ary.first
      else
        Integer(i)
      end
    end

    result = tokens.map do |token|
      case token.type
      when :str
        token.datum
      when :field
        arg = if token.arg_name
                get_named_argument.(token.arg_name)
              elsif token.arg_position
                get_positional_argument.(token.arg_position)
              else
                next_argument.()
              end
        token.precision ||= 6
        if token.flags.include?(:width_given_as_arg)
          token.width = arg
          arg = next_argument.()
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
                b = convert_int.(arg).to_s(2)
                b = "0b#{b}" if token.flags.include?(:alternate_format) && b != '0'
                apply_number_flags.(b, token.flags)
              when 'B'
                b = convert_int.(arg).to_s(2)
                b = "0B#{b}" if token.flags.include?(:alternate_format) && b != '0'
                apply_number_flags.(b, token.flags)
              when 'c'
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
              when 'd', 'u', 'i'
                d = convert_int.(arg).to_s
                apply_number_flags.(d, token.flags)
              when 'e', 'E', 'f', 'g', 'G'
                f = if arg.is_a?(Float)
                      arg.to_s
                    elsif arg.respond_to?(:to_ary)
                      arg.to_ary.first.to_s
                    else
                      Float(arg).to_s
                    end
                f = apply_number_flags.(f, token.flags)
                f << '.0' unless f.index('.')
                f << '0' until f.split('.').last.size >= token.precision
                f
              when 'o'
                o = convert_int.(arg).to_s(8)
                o = "0#{o}" if token.flags.include?(:alternate_format) && o != '0'
                apply_number_flags.(o, token.flags)
              when 'p'
                arg.inspect
              when 's'
                arg.to_s
              when 'x'
                x = convert_int.(arg).to_s(16)
                x = "0x#{x}" if token.flags.include?(:alternate_format) && x != '0'
                apply_number_flags.(x, token.flags)
              when 'X'
                x = convert_int.(arg).to_s(16).upcase
                x = "0X#{x}" if token.flags.include?(:alternate_format) && x != '0'
                apply_number_flags.(x, token.flags)
              else
                raise ArgumentError, "malformed format string - %#{token.datum}"
              end
        pad_char = token.flags.include?(:zero_padded) ? '0' : ' '
        while token.width && val.size < token.width
          val = pad_char + val
        end
        if token.width && token.width < 0
          val << pad_char while val.size < token.width.abs
        end
        val
      else
        raise "unknown token: #{token.inspect}"
      end
    end.join

    if $DEBUG && arguments.any? && !positional_argument_used
      raise ArgumentError, "too many arguments for format string"
    end

    result
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
