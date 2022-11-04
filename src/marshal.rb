module Marshal
  MAJOR_VERSION = 4
  MINOR_VERSION = 8

  class << self
    def dump(object, io = nil, limit = -1)
      if io.nil?
        writer = StringWriter.new
      else
        writer = Writer.new(io)
      end
      writer.write_version
      writer.write(object)
    end
  end

  class Writer
    def initialize(output)
      @output = output
    end

    def write_byte(value)
      @output.ungetbyte(value)
    end

    def write_bytes(value)
      @output.write(value)
    end

    def write_version
      write_byte(MAJOR_VERSION)
      write_byte(MINOR_VERSION)
    end

    def write_integer_bytes(value)
      if value.zero?
        write_byte(0)
      elsif value > 0 && value < 123
        write_byte(value + 5)
      elsif value > -124 && value < 0
        write_byte((value - 5) & 255)
      else
        buffer = []
        until value.zero? || value == -1
          buffer << (value & 255)
          value = value >> 8
        end
        if value.zero?
          write_byte(buffer.length)
        elsif value == -1
          write_byte(256 - buffer.length)
        end
        buffer.each do |integer|
          write_byte(integer)
        end
      end
    end

    def write_string_bytes(value)
      string = value.to_s
      write_integer_bytes(string.length)
      write_bytes(string)
    end

    def write_encoding_bytes(value)
      case value.encoding
      when Encoding::US_ASCII
        write_integer_bytes(1)
        write_symbol(:E)
        write_false
      when Encoding::UTF_8
        write_integer_bytes(1)
        write_symbol(:E)
        write_true
      end
    end

    def write_char(string)
      write_byte(string[0])
    end

    def write_nil
      write_char('0')
    end

    def write_true
      write_char('T')
    end

    def write_false
      write_char('F')
    end

    def write_integer(value)
      write_char('i')
      write_integer_bytes(value)
    end

    def write_string(value)
      write_char('I')
      write_char('"')
      write_string_bytes(value)
      write_encoding_bytes(value)
    end

    def write_symbol(value)
      write_char(':')
      write_string_bytes(value)
    end

    def write_float(value)
      write_char('f')
      if value.nan?
        write_string_bytes('nan')
      elsif value.infinite?
        write_string_bytes(value < 0 ? '-inf' : 'inf')
      elsif value == 0.0
        write_string_bytes(1 / value < 0 ? '-0' : '0')
      else
        write_string_bytes(value)
      end
    end

    def write_array(values)
      write_char('[')
      write_integer_bytes(values.size)
      values.each do |value|
        write(value)
      end
    end

    def write_hash(values)
      write_char('{')
      write_integer_bytes(values.size)
      values.each do |key, value|
        write(key)
        write(value)
      end
    end

    def write_class(value)
      write_char('c')
      write_string_bytes(value.name)
    end

    def write_module(value)
      write_char('m')
      write_string_bytes(value.name)
    end

    def write_regexp(value)
      write_char('I')
      write_string_bytes(value.source)
      write_byte(value.options)
      write_encoding_bytes(value)
    end

    def write_user_marshaled_object(value)
      write_char('U')
      write(value.class.name.to_sym)
      write(value.send(:marshal_dump))
    end

    def write_object(value)
      write_char('o')
      write(value.class.name.to_sym)
      ivar_names = value.instance_variables
      write_integer_bytes(ivar_names.size)
      ivar_names.each do |ivar_name|
        write(ivar_name)
        write(value.instance_variable_get(ivar_name))
      end
    end

    def write(value)
      if value.nil?
        write_nil
      elsif value.is_a?(TrueClass)
        write_true
      elsif value.is_a?(FalseClass)
        write_false
      elsif value.is_a?(Integer)
        write_integer(value)
      elsif value.is_a?(String)
        write_string(value)
      elsif value.is_a?(Symbol)
        write_symbol(value)
      elsif value.is_a?(Float)
        write_float(value)
      elsif value.is_a?(Array)
        write_array(value)
      elsif value.is_a?(Hash)
        write_hash(value)
      elsif value.is_a?(Class)
        write_class(value)
      elsif value.is_a?(Module)
        write_module(value)
      elsif value.is_a?(Regexp)
        write_regexp(value)
      elsif value.respond_to?(:marshal_dump, true)
        write_user_marshaled_object(value)
      elsif value.is_a?(Object)
        write_object(value)
      else
        raise TypeError, "can't dump #{value.class}"
      end

      @output
    end
  end

  class StringWriter < Writer
    def initialize
      @output = String.new.force_encoding(Encoding::ASCII_8BIT)
    end

    def write_byte(value)
      @output << value
    end

    def write_bytes(value)
      @output.concat(value)
    end
  end
end
