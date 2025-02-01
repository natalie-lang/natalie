# frozen_string_literal: true

require 'natalie/inline'

__inline__ 'extern "C" char *dtoa(double d, int mode, int ndigits, int *decpt, int *sign, char **rve);'

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

    def load(source)
      if source.respond_to?(:to_str)
        reader = StringReader.new(source.to_str)
      elsif source.respond_to?(:getbyte) && source.respond_to?(:read)
        reader = Reader.new(source)
      else
        raise TypeError, 'instance of IO needed'
      end
      reader.read_version
      reader.read_value
    end

    alias restore load
  end

  class Writer
    def initialize(output)
      @output = output
      @symbol_lookup = {}
      @object_lookup = {}
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

    def write_bigint_bytes(value)
      if value.positive?
        write_byte('+')
      else
        write_byte('-')
        value = value.abs
      end
      buffer = []
      until value.zero?
        buffer << (value & 0xff)
        value >>= 8
      end
      buffer << 0 if buffer.size.odd?
      write_integer_bytes(buffer.size / 2)
      buffer.each do |integer|
        write_byte(integer)
      end
    end

    def write_string_bytes(value)
      string = value.to_s
      write_integer_bytes(string.bytesize)
      write_bytes(string)
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
      if value >= -2 ** 30 && value < 2 ** 30
        write_char('i')
        write_integer_bytes(value)
      else
        write_char('l')
        write_bigint_bytes(value)
      end
    end

    def write_string(value, ivars)
      add_encoding_to_ivars(value, ivars)
      write_char('I') unless ivars.empty?
      write_char('"')
      write_string_bytes(value)
      write_ivars(ivars) unless ivars.empty?
    end

    def write_symbol(value)
      if @symbol_lookup.key?(value)
        write_char(';')
        write_integer_bytes(@symbol_lookup[value])
      else
        write_char(':')
        write_string_bytes(value)
        @symbol_lookup[value] = @symbol_lookup.size
      end
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
        __inline__ <<-END
            int decpt, sign;
            char *out, *e;
            out = dtoa(value_var->as_float()->to_double(), 0, 0, &decpt, &sign, &e);
            int digits = strlen(out);
            String string;
            if (decpt < -3 || decpt > digits) {
                string.append(out);
                string.insert(1, '.');
                string.append_sprintf("e%d", decpt - 1);
            } else if (decpt > 0) {
                string.append(out);
                if (decpt < digits) {
                    string.insert(decpt, '.');
                }
            } else {
                string = String("0.");
                string.append(::abs(decpt), '0');
                string.append(out);
            }
            if (sign) {
                string.prepend_char('-');
            }
            self->send(env, "write_string_bytes"_s, { new StringObject { string, Encoding::US_ASCII } });
        END
      end
    end

    def write_object_link(index)
      write_char('@')
      write_integer_bytes(index)
    end

    def write_array(values, ivars)
      write_char('I') unless ivars.empty?
      write_char('[')
      write_integer_bytes(values.size)
      values.each do |value|
        write(value)
      end
      write_ivars(ivars) unless ivars.empty?
    end

    def write_hash(values, ivars)
      write_char('I') unless ivars.empty?
      if values.default.nil?
        write_char('{')
      else
        write_char('}')
      end
      write_integer_bytes(values.size)
      values.each do |key, value|
        write(key)
        write(value)
      end
      unless values.default.nil?
        write(values.default)
      end
      write_ivars(ivars) unless ivars.empty?
    end

    def write_class(value)
      raise TypeError, "singleton class can't be dumped" if value.singleton_class?
      write_char('c')
      write_string_bytes(value.name)
    end

    def write_module(value)
      raise TypeError, "can't dump anonymous module #{value}" if value.name.nil?
      write_char('m')
      write_string_bytes(value.name)
    end

    def write_regexp(value, ivars)
      add_encoding_to_ivars(value, ivars)
      write_char('I')
      write_char('/')
      write_string_bytes(value.source)
      write_byte(value.options)
      write_ivars(ivars)
    end

    def write_data(value)
      raise TypeError, "can't dump anonymous class #{value.class}" if value.class.name.nil?
      write_char('S')
      write(value.class.to_s.to_sym)
      values = value.to_h
      write_integer_bytes(values.size)
      values.each do |name, value|
        write(name)
        write(value)
      end
    end

    def write_struct(value, ivars)
      raise TypeError, "can't dump anonymous class #{value.class}" if value.class.name.nil?
      values = value.to_h
      ivars.delete_if { |key, _| values.key?(key) }
      write_char('I') unless ivars.empty?
      write_char('S')
      write(value.class.to_s.to_sym)
      write_integer_bytes(values.size)
      values.each do |name, value|
        write(name)
        write(value)
      end
      write_ivars(ivars) unless ivars.empty?
    end

    def write_exception(value, ivars)
      message = value.message
      if message == value.class.inspect
        message = nil
      elsif message.ascii_only?
        message = message.b
      end
      ivars.prepend([:cause, value.cause]) if value.cause
      ivars.prepend([:bt, value.backtrace])
      ivars.prepend([:mesg, message])
      write_object(value, ivars)
    end

    def write_range(value, ivars)
      ivars.concat([
        [:excl, value.exclude_end?],
        [:begin, value.begin],
        [:end, value.end],
      ])
      write_object(value, ivars)
    end

    def write_user_marshaled_object_with_allocate(value)
      write_char('U')
      write(value.class.to_s.to_sym)
      write(value.send(:marshal_dump))
    end

    def write_user_marshaled_object_without_allocate(value)
      raise TypeError, "can't dump anonymous class #{value.class}" if value.class.name.nil?
      write_char('u')
      write(value.class.to_s.to_sym)
      dump = value.send(:_dump, -1)
      raise TypeError, '_dump() must return string' unless dump.is_a?(String)
      write_integer_bytes(dump.size)
      write_bytes(value.send(:_dump, -1))
    end

    def write_object(value, ivars)
      raise TypeError, "can't dump anonymous class #{value.class}" if value.class.name.nil?
      write_char('o')
      write(value.class.name.to_sym)
      write_ivars(ivars)
    end

    def write_ivars(ivars)
      write_integer_bytes(ivars.size)
      ivars.each do |ivar_name, ivar_value|
        write(ivar_name)
        write(ivar_value)
      end
    end

    def add_encoding_to_ivars(value, ivars)
      case value.encoding
      when Encoding::ASCII_8BIT
        nil # no encoding saved
      when Encoding::US_ASCII
        ivars.prepend([:E, false])
      when Encoding::UTF_8
        ivars.prepend([:E, true])
      else
        ivars.prepend([:encoding, value.encoding.name.b])
      end
    end

    def write(value)
      if value.respond_to?(:object_id) && @object_lookup.key?(value.object_id)
        write_object_link(@object_lookup.fetch(value.object_id))
        return @output
      elsif value.is_a?(Float) && @object_lookup.key?(value)
        write_object_link(@object_lookup.fetch(value))
        return @output
      end

      if !value.nil? && !value.is_a?(TrueClass) && !value.is_a?(FalseClass) && !value.is_a?(Integer) && !value.is_a?(Float) && !value.is_a?(Symbol)
        @object_lookup[value.object_id] = @object_lookup.size
      elsif value.is_a?(Float)
        @object_lookup[value] = @object_lookup.size
      end

      ivars = value.instance_variables.map { |ivar_name| [ivar_name, value.instance_variable_get(ivar_name)] }

      if value.nil?
        write_nil
      elsif value.is_a?(TrueClass)
        write_true
      elsif value.is_a?(FalseClass)
        write_false
      elsif value.is_a?(Integer)
        write_integer(value)
      elsif value.is_a?(String)
        write_string(value, ivars)
      elsif value.is_a?(Symbol)
        write_symbol(value)
      elsif value.is_a?(Float)
        write_float(value)
      elsif value.is_a?(Array)
        write_array(value, ivars)
      elsif value.is_a?(Hash)
        write_hash(value, ivars)
      elsif value.is_a?(Class)
        write_class(value)
      elsif value.is_a?(Module)
        write_module(value)
      elsif value.is_a?(Regexp)
        write_regexp(value, ivars)
      elsif value.is_a?(Data)
        write_data(value)
      elsif value.is_a?(Struct)
        write_struct(value, ivars)
      elsif value.is_a?(Exception)
        write_exception(value, ivars)
      elsif value.is_a?(Range)
        write_range(value, ivars)
      elsif value.respond_to?(:marshal_dump, true)
        write_user_marshaled_object_with_allocate(value)
      elsif value.respond_to?(:_dump, true)
        write_user_marshaled_object_without_allocate(value)
      elsif value.is_a?(Mutex) || value.is_a?(Proc) || value.is_a?(Method) || (defined?(StringIO) && value.is_a?(StringIO))
        raise TypeError, "no _dump_data is defined for class #{value.class}"
      elsif value.is_a?(MatchData) || value.is_a?(IO)
        raise TypeError, "can't dump #{value.class}"
      elsif value.is_a?(Object)
        write_object(value, ivars)
      else
        raise TypeError, "can't dump #{value.class}"
      end

      @output
    end
  end

  class StringWriter < Writer
    def initialize
      super(String.new.force_encoding(Encoding::ASCII_8BIT))
    end

    def write_byte(value)
      @output << value
    end

    def write_bytes(value)
      @output.concat(value.b)
    end
  end

  class Reader
    def initialize(source)
      @source = source
      @symbol_lookup = []
      @object_lookup = []
    end

    def read_byte
      byte = @source.getbyte
      raise ArgumentError, 'marshal data too short' if byte.nil?

      byte
    end

    def read_bytes(integer)
      bytes = @source.read(integer)
      raise ArgumentError, 'marshal data too short' if bytes.size < integer

      bytes
    end

    def read_version
      major = read_byte
      minor = read_byte

      if major != MAJOR_VERSION || minor > MINOR_VERSION
        raise TypeError, 'incompatible marshal file format'
      end
    end

    def read_object_link
      index = read_integer
      @object_lookup.fetch(index)
    end

    def read_signed_byte
      byte = read_byte
      byte > 127 ? (byte - 256) : byte
    end

    def read_integer
      byte = read_signed_byte

      if byte == 0
        return 0
      elsif byte > 0
        if byte > 4 && byte < 128
          return byte - 5
        end
        integer = 0
        byte.times do |i|
          integer |= read_byte << (8 * i)
        end
        integer
      else
        if byte > -129 && byte < -4
          return byte + 5
        end
        byte = -byte
        integer = -1
        byte.times do |i|
          integer &= ~(255 << (8 * i))
          integer |= read_byte << (8 * i)
        end
        integer
      end
    end

    def read_big_integer
      sign = read_byte.chr
      raise TypeError, 'incompatible marshal file format' unless ['+', '-'].include?(sign)
      size = read_integer
      integer = 0
      (2 * size).times do |i|
        integer |= read_byte << (8 * i)
      end
      integer = -integer if sign == '-'
      integer
    end

    def read_string
      integer = read_integer
      return +'' if integer.zero?
      read_bytes(integer)
    end

    def read_symbol
      symbol = read_string.to_sym
      @symbol_lookup << symbol
      symbol
    end

    def read_symbol_link
      link = read_integer
      @symbol_lookup.fetch(link)
    end

    def read_float
      string = read_string
      case string
      when 'nan' then Float::NAN
      when 'inf' then Float::INFINITY
      when '-inf' then -Float::INFINITY
      when '0' then 0.0
      when '-0' then -0.0
      else
        string.to_f
      end
    end

    def read_array
      result = []
      size = read_integer
      size.times { result << read_value }
      result
    end

    def read_hash
      result = {}
      size = read_integer
      size.times { result[read_value] = read_value }
      result
    end

    def read_hash_with_default
      hash = read_hash
      hash.default = read_value
      hash
    end

    def read_class
      name = read_string
      result = find_constant(name)
      unless result.instance_of?(Class)
        raise ArgumentError, "#{name} does not refer to class"
      end
      result
    end

    def read_module
      name = read_string
      result = find_constant(name)
      unless result.instance_of?(Module)
        raise ArgumentError, "#{name} does not refer to module"
      end
      result
    end

    def read_regexp
      string = read_string
      options = read_byte
      read_ivars(string)
      Regexp.new(string, options)
    end

    def read_user_marshaled_object_with_allocate
      name = read_value
      object_class = find_constant(name)
      data = read_value
      return Complex(data[0], data[1]) if object_class == Complex
      return Rational(data[0], data[1]) if object_class == Rational
      object = object_class.allocate
      unless object.respond_to?(:marshal_load)
        raise TypeError, "instance of #{object_class} needs to have method `marshal_load'"
      end
      object.marshal_load(data)
      object
    end

    def read_user_marshaled_object_without_allocate
      name = read_value
      object_class = find_constant(name)
      unless object_class.respond_to?(:_load)
        raise TypeError, "#{object_class} needs to have method `_load'"
      end
      data = read_string
      object_class._load(data)
    end

    def read_struct
      name = read_value
      object_class = find_constant(name)
      size = read_integer
      values = size.times.each_with_object({}) do |_, tmp|
        name = read_value
        value = read_value
        tmp[name] = value
      end
      if object_class.ancestors.include?(Data)
        object_class.new(**values)
      else
        object = object_class.allocate
        values.each { |k, v| object.send(:"#{k}=", v) }
        object
      end
    end

    def read_object
      name = read_value
      object_class = find_constant(name)
      object = object_class.allocate
      ivars_hash = read_hash
      if object_class == Range
        object = Range.new(ivars_hash.delete(:begin), ivars_hash.delete(:end), ivars_hash.delete(:excl))
      elsif object.is_a?(Exception)
        object = object_class.new(ivars_hash.delete(:mesg))
        object.set_backtrace(ivars_hash.delete(:bt)) if ivars_hash.key?(:bt)
        if ivars_hash.key?(:cause)
          exception = object
          cause = ivars_hash.delete(:cause)
          __inline__ 'exception_var->as_exception_or_raise(env)->set_cause(cause_var->as_exception_or_raise(env));'
        end
      end
      ivars_hash.each do |ivar_name, value|
        object.instance_variable_set(ivar_name, value)
      end
      object
    end

    def read_ivars(object)
      read_hash.each do |name, value|
        if name == :E
          if value == false
            object.force_encoding(Encoding::US_ASCII)
          elsif value == true
            object.force_encoding(Encoding::UTF_8)
          end
        elsif name == :encoding
          object.force_encoding(value)
        else
          object.instance_variable_set(name, value)
        end
      end
    end

    def read_value
      char = read_byte.chr
      case char
      when '0'
        nil
      when 'T'
        true
      when 'F'
        false
      when '@'
        read_object_link
      when 'i'
        read_integer
      when 'l'
        read_big_integer
      when ':'
        read_symbol
      when ';'
        read_symbol_link
      when 'f'
        read_float
      when 'I'
        result = read_value
        read_ivars(result) unless result.is_a?(Regexp)
        result
      else
        index = @object_lookup.size
        @object_lookup << nil # placeholder
        value = case char
                when '"'
                  read_string
                when '['
                  read_array
                when '{'
                  read_hash
                when '}'
                  read_hash_with_default
                when 'c'
                  read_class
                when 'm'
                  read_module
                when '/'
                  read_regexp
                when 'U'
                  read_user_marshaled_object_with_allocate
                when 'u'
                  read_user_marshaled_object_without_allocate
                when 'S'
                  read_struct
                when 'o'
                  read_object
                else
                  raise ArgumentError, 'dump format error'
                end
        @object_lookup[index] = value
        value
      end
    end

    def find_constant(name)
      begin
        # NATFIXME: Should be supported directly in const_get
        name.to_s.split('::').reduce(Object) { |memo, part| memo.const_get(part) }
      rescue NameError
        raise ArgumentError, "undefined class/module #{name}"
      end
    end
  end

  class StringReader < Reader
    def initialize(source)
      super(source)
      @offset = 0
    end

    def read_byte
      byte = @source.getbyte(@offset)
      raise ArgumentError, 'marshal data too short' if byte.nil?

      @offset += 1
      byte
    end

    def read_bytes(integer)
      if @source.bytesize - @offset >= integer
        string = @source.byteslice(@offset, integer)
        @offset += integer
        string.b
      else
        raise ArgumentError, 'marshal data too short'
      end
    end
  end
end
