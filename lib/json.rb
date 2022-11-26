module JSON
  class JSONError < StandardError; end
  class ParserError < JSONError; end

  def self.generate(object)
    Generator.new(object).generate
  end

  def self.parse(string, **options)
    tokens = Lexer.new(string).tokens
    Parser.new(tokens, **options).parse
  end

  class Generator
    def initialize(object)
      @object = object
      @string = +''
    end

    def generate
      generate_element(@object)
      @string
    end

    private

    def generate_element(value)
      case value
      when NilClass
        @string << 'null'
      when TrueClass
        @string << 'true'
      when FalseClass
        @string << 'false'
      when String, Symbol
        generate_string(value)
      when Array
        generate_array(value)
      when Hash
        generate_object(value)
      else
        @string << value.to_s
      end
    end

    def generate_string(value)
      @string << '"'
      if value.is_a?(Symbol)
        @string << value.to_s
      else
        @string << escape_string(value)
      end
      @string << '"'
    end

    def generate_array(values)
      @string << '['
      generate_each(values) { |value| generate_element(value) }
      @string << ']'
    end

    def generate_object(values)
      @string << '{'
      generate_each(values) { |key, value| generate_key_value_pair(key, value) }
      @string << '}'
    end

    def generate_each(enumerable, &block)
      index = 0
      enumerable.each do |*args|
        @string << ',' unless index.zero?
        block.call(*args)
        index += 1
      end
    end

    def generate_key_value_pair(key, value)
      generate_string(key)
      @string << ':'
      generate_element(value)
    end

    def escape_string(value)
      result = +''
      value.to_s.codepoints.each do |codepoint|
        result << case codepoint
        when 8 then '\\b'
        when 9 then '\\t'
        when 10 then '\\n'
        when 12 then '\\f'
        when 13 then '\\r'
        when 34 then '\\"'
        when 92 then '\\\\'
        when 0...32
          '\\u' + codepoint.to_s(16).rjust(4, '0')
        else
          codepoint
        end
      end
      result
    end
  end

  class Lexer
    DIGITS = '0'..'9'
    HEX_DIGITS = DIGITS.to_a + ('a'..'f').to_a + ('A'..'F').to_a

    def initialize(string)
      @string = string
      @remaining_string = @string.dup
      @index = 0
    end

    def tokens
      skip_whitespace
      ary = []
      while (token = next_token)
        ary << token
        skip_whitespace
      end
      ary
    end

    private

    def next_token
      return nil if @remaining_string.empty?

      case (c = current_char)
      when 't'
        raise ParserError, "unknown literal: #{c}" unless advance(4) == 'true'
        :true # rubocop:disable Lint/BooleanSymbol
      when 'f'
        raise ParserError, "unknown literal: #{c}" unless advance(5) == 'false'
        :false # rubocop:disable Lint/BooleanSymbol
      when 'n'
        raise ParserError, "unknown literal: #{c}" unless advance(4) == 'null'
        :null
      when '-', DIGITS
        num = advance
        while DIGITS.include?(c = current_char)
          num << c
          advance
        end
        case c
        when '.'
          num << c
          advance
          while DIGITS.include?(c = current_char)
            num << c
            advance
          end
          if c == 'e' || c == 'E'
            num << c
            advance
            while DIGITS.include?(c = current_char)
              num << c
              advance
            end
          end
          num.to_f
        when 'e', 'E'
          num << c
          advance
          while DIGITS.include?(c = current_char)
            num << c
            advance
          end
          num.to_f
        else
          if (num.start_with?('0') && num.length > 1) || (num.start_with?('-0') && num.length > 2)
            raise ParserError, "unknown literal: #{num}"
          end
          num.to_i
        end
      when '"'
        str = ''
        advance
        loop do
          case (c = advance)
          when '"'
            return str
          when '\\'
            c2 = advance
            case c2
            when '"', '/'
              str << '"'
            when '\\'
              str << '\\'
            when 'b'
              str << "\b"
            when 'f'
              str << "\f"
            when 'r'
              str << "\r"
            when 'n'
              str << "\n"
            when 't'
              str << "\t"
            when 'u'
              escaped = ''
              while HEX_DIGITS.include?(current_char) && escaped.size < 4
                escaped << advance
              end
              if escaped.size != 4
                raise ParserError, 'incomplete unicode escape'
              end
              str << escaped.to_i(16).chr(Encoding::UTF_8)
            else
              str << c2
            end
          when "\n"
            raise ParserError, 'unexpected newline inside string'
          when nil
            raise ParserError, 'unexpected end of input'
          else
            str << c
          end
        end
      when '[', ']', '{', '}', ',', ':'
        advance.to_sym
      else
        raise ParserError, "unknown sequence at index #{@index}: #{@remaining_string[0..10].inspect}"
      end
    end

    def skip_whitespace
      advance while [' ', "\n", "\t", "\r"].include?(current_char)
    end

    def current_char
      return nil if @remaining_string.empty?
      @remaining_string[0]
    end

    def advance(count = 1)
      return nil if @remaining_string.empty?
      result = @remaining_string[...count]
      @index += count
      @remaining_string = @remaining_string[count..]
      result
    end
  end

  class Parser
    def initialize(tokens, symbolize_names: false)
      @tokens = tokens
      @symbolize_names = symbolize_names
    end

    def parse
      result = parse_sequence(@tokens.shift)
      if @tokens.any?
        raise ParserError, "unexpected token: #{@tokens.first.inspect}"
      end
      result
    end

    private

    def parse_sequence(token)
      case token
      when :true # rubocop:disable Lint/BooleanSymbol
        true
      when :false # rubocop:disable Lint/BooleanSymbol
        false
      when :null
        nil
      when Float
        token
      when Integer
        token
      when String
        token
      when :'['
        parse_array
      when :'{'
        parse_object
      else
        raise ParserError, "unknown token: #{token.inspect}"
      end
    end

    def parse_array
      ary = []
      case (token = @tokens.shift)
      when nil
        raise ParserError, 'expected array'
      when :']'
        :noop
      else
        ary << parse_sequence(token)
        while (token = @tokens.shift) && token == :','
          ary << parse_sequence(@tokens.shift)
        end
        raise ParserError, "expected ], but got: #{token.inspect}" unless token == :']'
      end
      ary
    end

    def parse_object
      obj = {}
      case (token = @tokens.shift)
      when nil
        raise ParserError, 'expected object'
      when :'}'
        :noop
      else
        parse_key_value_pair(obj, token)
        while (token = @tokens.shift) && token == :','
          parse_key_value_pair(obj, @tokens.shift)
        end
        raise ParserError, "expected }, but got: #{token.inspect}" unless token == :'}'
      end
      obj
    end

    def parse_key_value_pair(obj, token)
      case token
      when String
        key = token
        key = key.to_sym if @symbolize_names
        token = @tokens.shift
        raise ParserError, ':' unless token == :':'
        token = @tokens.shift
        raise ParserError, 'expected object value' unless token
        value = parse_sequence(token)
        obj[key] = value
      else
        raise ParserError, "expected object string key, but got: #{token.inspect}"
      end
    end
  end
end
