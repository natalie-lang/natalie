class CSV
  class MalformedCSVError < RuntimeError
    attr_reader :line_number
    alias_method :lineno, :line_number

    def initialize(message, line_number)
      @line_number = line_number
      super("#{message} in line #{line_number}.")
    end
  end

  class Parser
    attr_reader :lineno

    class Scanner
      def initialize(input)
        @input = input
        @line_number = 0
        @eof = input.eof?
      end

      def eof?
        return false if @line && @current_index < @line.size

        @input.eof?
      end

      def advance
        peek.tap { @current_index += 1 }
      end

      def read_line
        @current_index = 0
        @line = @input.gets

        if @line
          @line_number += 1
        else
          @eof = true
        end

        @line
      end

      # Current char or nil if input is exhausted
      def peek
        return nil if @eof

        read_line if !@line || @current_index >= @line.size

        @line&.[](@current_index)
      end
    end

    def initialize(input, options)
      @input = input
      @options = options
      @lineno = 0
      @scanner = Scanner.new(input)
    end

    def header_row?
      @options[:headers] && !@headers
    end

    def next_line
      @headers = consume_line if header_row?

      line = consume_line

      if @headers && line
        Row.new(@headers, line)
      else
        line
      end
    end

    def headers
      return nil unless @options[:headers]

      @headers || true
    end

    def consume_line
      return nil if eof?

      @lineno += 1

      [].tap do |out|
        while !line_break? && !eof?
          advance if separator?
          out << next_col
        end
        advance # skip line break if there is one
      end
    end

    def next_col
      if quote?
        parse_quoted_column_value
      else
        parse_unquoted_column_value
      end
    end

    def parse_quoted_column_value
      advance
      ''.tap do |out|
        while true
          if quote?
            advance
            if !@options[:liberal_parsing] && !(separator? || eof? || line_break?)
              raise MalformedCSVError.new("Any value after quoted field isn't allowed", @lineno)
            end
            break
          elsif !@options[:liberal_parsing] && eof?
            raise MalformedCSVError.new('Unclosed quoted field', @lineno)
          else
            out << advance
          end
        end
      end
    end

    def parse_unquoted_column_value
      return nil if separator? || eof? || line_break?

      ''.tap do |out|
        while !(separator? || eof? || line_break?)
          raise MalformedCSVError.new('Illegal quoting', @lineno) if !@options[:liberal_parsing] && quote?

          out << advance
        end
      end
    end

    def advance
      @scanner.advance
    end

    def peek
      @scanner.peek
    end

    def line_break?
      peek == "\n"
    end

    def eof?
      @scanner.eof?
    end

    def separator?
      peek == @options[:col_sep]
    end

    def quote?
      peek == quote_character
    end

    def quote_character
      @options[:quote_char]
    end

    def detect_row_separator
      new_line = "\n"
      reset_line = "\r"
      while char = @input.getc
        if char == new_line
          return char
        elsif char == reset_line
          if @input.getc == new_line
            return reset_line + new_line
          else
            return reset_line
          end
        end
      end
    end
  end
end
