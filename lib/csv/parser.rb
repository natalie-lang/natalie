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

    def initialize(input, options)
      @input = input
      @options = options
      @line = nil
      @lineno = 0
      @current_col = 0
      @current_index = 0
    end

    def next_line
      line = consume_line

      if @lineno == 1 && @options[:headers]
        # We have just read the headers line
        @headers = line

        line = consume_line
      end

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
      @line = @input.gets
      return nil unless @line
      @current_col = 0
      @current_index = 0
      @lineno += 1

      [].tap do |out|
        while !line_break? && !eol?
          advance if separator?
          out << next_col
        end
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
      "".tap do |out|
        while true
          if quote?
            advance
            if !@options[:liberal_parsing] && !(separator? || eol? || line_break?)
              raise MalformedCSVError.new("Any value after quoted field isn't allowed", @lineno)
            end
            break
          elsif !@options[:liberal_parsing] && (separator? || eol? || line_break?)
            raise MalformedCSVError.new('Unclosed quoted field', @lineno)
          else
            out << advance
          end
        end
      end
    end

    def parse_unquoted_column_value
      if separator? || eol? || line_break?
        return nil
      end

      "".tap do |out|
        while !(separator? || eol? || line_break?)
          if !@options[:liberal_parsing] && quote?
            raise MalformedCSVError.new('Illegal quoting', @lineno)
          end

          out << advance
        end
      end
    end

    def advance
      peek.tap { @current_index += 1 }
    end

    def peek
      @line[@current_index]
    end

    def line_break?
      peek == "\n"
    end

    def eol?
      !peek
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
