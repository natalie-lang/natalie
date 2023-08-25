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

    TRANSITIONS = {
      literal: {
        default: :literal,
        on_percent: :field_beginning,
      },
      literal_percent: {
        default: :literal,
      },
      field_flag: {
        on_alpha: :field_end,
        on_newline: :literal_percent,
        on_null_byte: :literal_percent,
        on_number: :field_width,
        on_percent: :literal,
      },
      field_beginning: {
        on_alpha: :field_end,
        on_asterisk: :field_flag,
        on_minus: :field_width_minus,
        on_newline: :literal_percent,
        on_null_byte: :literal_percent,
        on_number: :field_width,
        on_percent: :literal,
        on_plus: :field_flag,
        on_pound: :field_flag,
        on_space: :field_flag,
        on_zero: :field_flag,
      },
      field_end: {
        default: :literal,
        on_percent: :field_beginning,
      },
      field_precision: {
        on_alpha: :field_end,
        on_number: :field_precision,
      },
      field_precision_period: {
        on_number: :field_precision,
      },
      field_width: {
        on_alpha: :field_end,
        on_dollar: :positional_argument,
        on_number: :field_width,
        on_period: :field_precision_period,
        on_zero: :field_width,
      },
      field_width_minus: {
        on_number: :field_width,
      },
      positional_argument: {
        on_alpha: :field_end,
        on_number: :field_width,
        on_period: :field_precision_period,
        on_zero: :field_width,
      }
    }.freeze

    COMPLETE_STATES = %i[
      field_end
      literal
      literal_percent
    ].freeze

    class Token
      def initialize(type:, datum:, flags: nil, width: nil, precision: nil, arg_position: nil)
        @type = type
        @datum = datum
        @flags = flags
        @width = width
        @precision = precision
        @arg_position = arg_position
      end

      attr_accessor :type, :datum, :flags, :width, :precision, :arg_position
    end

    def tokens
      state = :literal
      width = nil
      width_minus = false
      precision = nil
      arg_position = nil
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
                     when '1'..'9'
                       :on_number
                     when 'a'..'z', 'A'..'Z'
                       :on_alpha
                     end

        new_state = TRANSITIONS.dig(state, transition) || TRANSITIONS.dig(state, :default)

        #puts "#{state.inspect}, given #{char.inspect}, " \
             #"transition #{transition.inspect} to #{new_state.inspect}"

        unless new_state
          raise ArgumentError, "no transition from #{state.inspect} with char #{char.inspect}"
        end

        state = new_state
        next_char

        case state
        when :literal
          tokens << Token.new(type: :str, datum: char)
        when :literal_percent
          tokens << Token.new(type: :str, datum: "%#{char}")
        when :field_beginning, :field_precision_period
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
                     :width_given_as_arg
                   else
                     raise ArgumentError, "unknown flag: #{char.inspect}"
                   end
        when :field_width
          width = (width || 0) * 10 + char.to_i
          width *= -1 if width_minus
          width_minus = false
        when :field_width_from_arg
          width = :from_arg
        when :field_width_minus
          width_minus = true
        when :field_precision
          precision = (precision || 0) * 10 + char.to_i
        when :field_end
          tokens << Token.new(
            type: :field,
            datum: char,
            flags: flags.dup,
            width: width,
            precision: precision,
            arg_position: arg_position
          )
          width = nil
          precision = nil
          arg_position = nil
        when :positional_argument
          arg_position = width
          width = nil
        else
          raise ArgumentError, "unknown state: #{state.inspect}"
        end
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

    result = tokens.map do |token|
      case token.type
      when :str
        token.datum
      when :field
        arg = arguments.shift
        token.precision ||= 6
        if token.flags.include?(:width_given_as_arg)
          token.width = arg
          arg = arguments.shift
        end
        val = case token.datum
              when 'b'
                b = arg.to_s(2)
                apply_number_flags.(b, token.flags)
              when 's'
                arg.to_s
              when 'd', 'u'
                d = arg.to_i.to_s
                apply_number_flags.(d, token.flags)
              when 'f'
                f = arg.to_f.to_s
                f = apply_number_flags.(f, token.flags)
                f << '.0' unless f.index('.')
                f << '0' until f.split('.').last.size >= token.precision
                f
              when 'x'
                x = arg.to_i.to_s(16)
                x = "0x#{x}" if token.flags.include?(:alternate_format) && x != '0'
                apply_number_flags.(x, token.flags)
              when 'X'
                x = arg.to_i.to_s(16).upcase
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

    if $DEBUG && arguments.any?
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
