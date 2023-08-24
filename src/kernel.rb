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

    def tokens
      tokens = []

      while index < chars.size
        c = current_char
        if c == '%'
          next_char
          flags = consume_flags
          case (c = current_char)
          when '%'
            tokens << [:str, '%']
          when 'a'..'z', 'A'..'Z'
            tokens << [:field, c, flags]
          when "\n", "\0"
            tokens << [:str, "%#{c}"]
          when '1'..'9'
            width = consume_number
            case (c = current_char)
            when 'a'..'z', 'A'..'Z'
              tokens << [:field, c, flags, width]
            when '.'
              next_char
              precision = consume_number
              case (c = current_char)
              when 'a'..'z', 'A'..'Z'
                tokens << [:field, c, flags, width, precision]
              else
                raise ArgumentError, 'invalid format character - %'
              end
            else
              raise ArgumentError, 'invalid format character - %'
            end
          when nil
            raise ArgumentError, 'incomplete format specifier; use %% (double %) instead'
          else
            raise ArgumentError, 'invalid format character - %'
          end
          next_char
        else
          start_index = index
          while chars[index] && chars[index] != '%'
            @index += 1
          end
          literal = chars[start_index...index].join
          tokens << [:str, literal]
        end
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

    def consume_number
      c = chars[index]
      raise ArgumentError, 'bad width' unless ('1'..'9').cover?(c)
      num = [c]
      while ('0'..'9').cover?(c = next_char)
        num << c
      end
      num.join.to_i
    end

    def consume_flags
      f = chars[index]
      flags = []
      while [' ', '#', '-', '+', '0', '*'].include?(f)
        case f
        when ' ' then flags << :space
        when '#' then flags << :alter
        when "0" then flags << :zero
        when "+" then flags << :plus
        when "-" then flags << :left
        when "*" then
          raise ArgumentError, "cannot specify '*' multiple times" if flags.include?(:star)
          flags << :star
        else
          raise NotImplementedError, 'invalid branch for consuming flags'
        end
        @index += 1
        f = chars[index]
      end
      flags
    end
  end

  def sprintf(format_string, *arguments)
    tokens = SprintfParser.new(format_string).tokens

    result = tokens.map do |token|
      case token.first
      when :str
        token.last
      when :field
        arg = arguments.shift
        _, type, flags, width, precision = token
        precision ||= 6
        val = case type
              when 'b'
                arg.to_s(2)
              when 's'
                arg.to_s
              when 'd', 'u'
                arg.to_i.to_s
              when 'f'
                f = arg.to_f.to_s
                f = " #{f}" if flags.include?(:space) && !f.start_with?('-')
                f << '.0' unless f.index('.')
                f << '0' until f.split('.').last.size >= precision
                f
              when 'x'
                arg.to_i.to_s(16)
              else
                raise ArgumentError, "malformed format string - %#{token[1]}"
              end
        while width && val.size < width
          val = ' ' + val
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
