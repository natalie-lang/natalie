# frozen_string_literal: true

require 'natalie/inline'

__inline__ 'extern "C" char *dtoa(double d, int mode, int ndigits, int *decpt, int *sign, char **rve);'

module Marshal
  MAJOR_VERSION = 4
  MINOR_VERSION = 8

  class << self
    def dump(object, io = nil, limit = -1)
      if io.is_a?(Integer)
        limit = io
        io = nil
      end
      if io.nil?
        writer = StringWriter.new(limit)
      else
        raise TypeError, 'instance of IO needed' unless io.respond_to?(:write)
        io.binmode if io.respond_to?(:binmode)
        writer = Writer.new(io, limit)
      end
      writer.write_version
      writer.write(object)
    end

    def load(source, proc = nil, freeze: false)
      if source.respond_to?(:to_str)
        reader = StringReader.new(source.to_str, proc, freeze: freeze)
      elsif source.respond_to?(:getbyte) && source.respond_to?(:read)
        reader = Reader.new(source, proc, freeze: freeze)
      else
        raise TypeError, 'instance of IO needed'
      end
      reader.read_version
      reader.read_value
    end

    alias restore load
  end

  class Writer
    def initialize(output, limit = -1)
      @output = output
      @initial_limit = limit
      @symbol_lookup = {}
      @object_lookup = {}
    end

    def write_byte(value)
      @output.write(value.chr)
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
        buffer.each { |integer| write_byte(integer) }
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
      buffer.each { |integer| write_byte(integer) }
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
      if value >= -2**30 && value < 2**30
        write_char('i')
        write_integer_bytes(value)
      else
        write_char('l')
        write_bigint_bytes(value)
      end
    end

    def write_string(value, ivars, limit)
      add_encoding_to_ivars(value, ivars)
      write_char('I') unless ivars.empty?
      write_extended_modules(value)
      write_user_class(value, String)
      write_char('"')
      write_string_bytes(value)
      write_ivars(ivars, limit) unless ivars.empty?
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
            out = dtoa(value_var.as_float()->to_double(), 0, 0, &decpt, &sign, &e);
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
            self.send(env, "write_string_bytes"_s, { StringObject::create(string, Encoding::US_ASCII) });
        END
      end
    end

    def write_object_link(index)
      write_char('@')
      write_integer_bytes(index)
    end

    def write_array(values, ivars, limit)
      write_char('I') unless ivars.empty?
      write_extended_modules(values)
      write_user_class(values, Array)
      write_char('[')
      write_integer_bytes(values.size)
      values.each { |value| write(value, limit) }
      write_ivars(ivars, limit) unless ivars.empty?
    end

    def write_hash(values, ivars, limit)
      raise TypeError, "can't dump hash with default proc" if values.default_proc
      write_char('I') unless ivars.empty?
      write_extended_modules(values)
      write_user_class(values, Hash)
      if values.compare_by_identity?
        write_char('C')
        write_symbol(:Hash)
      end
      if values.default.nil?
        write_char('{')
      else
        write_char('}')
      end
      write_integer_bytes(values.size)
      values.each do |key, value|
        write(key, limit)
        write(value, limit)
      end
      write(values.default, limit) unless values.default.nil?
      write_ivars(ivars, limit) unless ivars.empty?
    end

    def write_class(value)
      raise TypeError, "singleton class can't be dumped" if value.singleton_class?
      name = Module.instance_method(:name).bind_call(value)
      raise TypeError, "can't dump anonymous class #{value}" if name.nil?
      write_char('c')
      write_string_bytes(name)
    end

    def write_user_class(value, base)
      return if value.class == base
      name = Module.instance_method(:name).bind_call(value.class)
      raise TypeError, "can't dump anonymous class #{value.class}" if name.nil?
      write_char('C')
      write_symbol(name.to_sym)
    end

    def write_extended_modules(value)
      singleton = Kernel.instance_method(:singleton_class).bind_call(value)
      klass = Kernel.instance_method(:class).bind_call(value)
      extended = singleton.included_modules - klass.included_modules
      extended.reverse_each do |mod|
        name = Module.instance_method(:name).bind_call(mod)
        raise TypeError, "can't dump anonymous class #{mod}" if name.nil?
        write_char('e')
        write_symbol(name.to_sym)
      end
    end

    def write_module(value)
      name = Module.instance_method(:name).bind_call(value)
      raise TypeError, "can't dump anonymous module #{value}" if name.nil?
      write_char('m')
      write_string_bytes(name)
    end

    def write_regexp(value, ivars, limit)
      add_encoding_to_ivars(value, ivars)
      write_char('I') unless ivars.empty?
      write_extended_modules(value)
      write_user_class(value, Regexp)
      write_char('/')
      write_string_bytes(value.source)
      write_byte(value.options)
      write_ivars(ivars, limit) unless ivars.empty?
    end

    def write_data(value, limit)
      raise TypeError, "can't dump anonymous class #{value.class}" if value.class.name.nil?
      write_char('S')
      write(value.class.to_s.to_sym, limit)
      values = value.to_h
      write_integer_bytes(values.size)
      values.each do |name, value|
        write(name, limit)
        write(value, limit)
      end
    end

    def write_struct(value, ivars, limit)
      name = Module.instance_method(:name).bind_call(value.class)
      raise TypeError, "can't dump anonymous class #{value.class}" if name.nil?
      values = value.to_h
      ivars.delete_if { |key, _| values.key?(key) }
      write_char('I') unless ivars.empty?
      write_extended_modules(value)
      write_char('S')
      write(name.to_sym, limit)
      write_integer_bytes(values.size)
      values.each do |name, value|
        write(name, limit)
        write(value, limit)
      end
      write_ivars(ivars, limit) unless ivars.empty?
    end

    def write_exception(value, ivars, limit)
      message = value.message
      if message == value.class.inspect
        message = nil
      elsif message.ascii_only?
        message = message.b
      end
      ivars.prepend([:cause, value.cause]) if value.cause
      ivars.prepend([:bt, value.backtrace])
      ivars.prepend([:mesg, message])
      write_object(value, ivars, limit)
    end

    def write_range(value, ivars, limit)
      ivars.concat([[:excl, value.exclude_end?], [:begin, value.begin], [:end, value.end]])
      write_object(value, ivars, limit)
    end

    def write_user_marshaled_object_with_allocate(value, limit)
      klass = Kernel.instance_method(:class).bind_call(value)
      name = Module.instance_method(:name).bind_call(klass)
      raise TypeError, "can't dump anonymous class #{klass}" if name.nil?
      write_char('U')
      write(name.to_sym, limit)
      write(value.__send__(:marshal_dump), limit)
    end

    def write_user_marshaled_object_without_allocate(value, limit)
      klass = Kernel.instance_method(:class).bind_call(value)
      name = Module.instance_method(:name).bind_call(klass)
      raise TypeError, "can't dump anonymous class #{klass}" if name.nil?
      dump = value.__send__(:_dump, -1)
      raise TypeError, '_dump() must return string' unless String === dump
      dump_ivar_names = Kernel.instance_method(:instance_variables).bind_call(dump)
      dump_ivars = dump_ivar_names.map { |n| [n, Kernel.instance_method(:instance_variable_get).bind_call(dump, n)] }
      add_encoding_to_ivars(dump, dump_ivars)
      write_char('I') unless dump_ivars.empty?
      write_char('u')
      write(name.to_sym, limit)
      write_integer_bytes(dump.bytesize)
      write_bytes(dump)
      write_ivars(dump_ivars, limit) unless dump_ivars.empty?
      @object_lookup[Kernel.instance_method(:object_id).bind_call(value)] = @object_lookup.size
    end

    def write_object(value, ivars, limit)
      klass = Kernel.instance_method(:class).bind_call(value)
      singleton = Kernel.instance_method(:singleton_class).bind_call(value)
      if singleton.instance_methods(false).any? || singleton.instance_variables.any?
        raise TypeError, "singleton can't be dumped"
      end
      name = Module.instance_method(:name).bind_call(klass)
      raise TypeError, "can't dump anonymous class #{klass}" if name.nil?
      write_extended_modules(value)
      write_char('o')
      write(name.to_sym, limit)
      write_ivars(ivars, limit)
    end

    def write_ivars(ivars, limit)
      write_integer_bytes(ivars.size)
      ivars.each do |ivar_name, ivar_value|
        write(ivar_name, limit)
        write(ivar_value, limit)
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

    def write(value, limit = @initial_limit)
      raise ArgumentError, 'exceed depth limit' if limit == 0
      limit -= 1 if limit > 0
      klass = Kernel.instance_method(:class).bind_call(value)
      oid = Kernel.instance_method(:object_id).bind_call(value) unless Integer === value
      if oid && @object_lookup.key?(oid)
        write_object_link(@object_lookup.fetch(oid))
        return @output
      elsif Integer === value && (value >= 2**62 || value < -(2**62)) && @object_lookup.key?([:integer, value])
        write_object_link(@object_lookup.fetch([:integer, value]))
        return @output
      elsif Float === value && @object_lookup.key?(value)
        write_object_link(@object_lookup.fetch(value))
        return @output
      end

      has_respond_to = klass.method_defined?(:respond_to?)
      has_marshal_dump = has_respond_to && value.respond_to?(:marshal_dump, true)
      has_dump = has_respond_to && !has_marshal_dump && value.respond_to?(:_dump, true)

      if !nil.equal?(value) && !(TrueClass === value) && !(FalseClass === value) && !(Integer === value) &&
           !(Float === value) && !(Symbol === value) && !has_dump
        @object_lookup[oid] = @object_lookup.size
      elsif Integer === value && (value >= 2**30 || value < -(2**30))
        # Integers are special: Object links are only used when 64 bits are used, but the objects are counted when 32 bits are used
        @object_lookup[[:integer, value]] = @object_lookup.size
      elsif Float === value
        @object_lookup[value] = @object_lookup.size
      end

      ivar_names = Kernel.instance_method(:instance_variables).bind_call(value)
      ivars = ivar_names.map { |name| [name, Kernel.instance_method(:instance_variable_get).bind_call(value, name)] }

      if nil.equal?(value)
        write_nil
      elsif TrueClass === value
        write_true
      elsif FalseClass === value
        write_false
      elsif Integer === value
        write_integer(value)
      elsif String === value
        write_string(value, ivars, limit)
      elsif Symbol === value
        write_symbol(value)
      elsif Float === value
        write_float(value)
      elsif Array === value
        write_array(value, ivars, limit)
      elsif Hash === value
        write_hash(value, ivars, limit)
      elsif Class === value
        write_class(value)
      elsif Module === value
        write_module(value)
      elsif Regexp === value
        write_regexp(value, ivars, limit)
      elsif Data === value
        write_data(value, limit)
      elsif Struct === value
        write_struct(value, ivars, limit)
      elsif Exception === value
        write_exception(value, ivars, limit)
      elsif Range === value
        write_range(value, ivars, limit)
      elsif has_marshal_dump
        write_user_marshaled_object_with_allocate(value, limit)
      elsif has_dump
        write_user_marshaled_object_without_allocate(value, limit)
      elsif Mutex === value || Proc === value || Method === value ||
            (defined?(StringIO) && StringIO === value)
        raise TypeError, "no _dump_data is defined for class #{klass}"
      elsif MatchData === value || IO === value
        raise TypeError, "can't dump #{klass}"
      else
        write_object(value, ivars, limit)
      end

      @output
    end
  end

  class StringWriter < Writer
    def initialize(limit = -1)
      super(String.new.force_encoding(Encoding::ASCII_8BIT), limit)
    end

    def write_byte(value)
      @output << value
    end

    def write_bytes(value)
      @output.concat(value.b)
    end
  end

  class Reader
    def initialize(source, proc = nil, freeze: false)
      @source = source
      @proc = proc
      @freeze = freeze
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

      raise TypeError, 'incompatible marshal file format' if major != MAJOR_VERSION || minor > MINOR_VERSION
    end

    def read_object_link
      index = read_integer
      raise ArgumentError, 'dump format error' if index < 0 || index >= @object_lookup.size
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
        return byte - 5 if byte > 4 && byte < 128
        integer = 0
        byte.times { |i| integer |= read_byte << (8 * i) }
        integer
      else
        return byte + 5 if byte > -129 && byte < -4
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
      raise TypeError, 'incompatible marshal file format' unless %w[+ -].include?(sign)
      size = read_integer
      integer = 0
      (2 * size).times { |i| integer |= read_byte << (8 * i) }
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
      when 'nan'
        Float::NAN
      when 'inf'
        Float::INFINITY
      when '-inf'
        -Float::INFINITY
      when '0'
        0.0
      when '-0'
        -0.0
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
      raise ArgumentError, "#{name} does not refer to class" unless result.instance_of?(Class)
      result
    end

    def read_module
      name = read_string
      result = find_constant(name)
      raise ArgumentError, "#{name} does not refer to module" unless result.instance_of?(Module)
      result
    end

    def read_regexp
      string = read_string
      options = read_byte
      read_ivars(string)
      Regexp.new(string, options)
    end

    def read_extended(ivars_consumed)
      name = read_value
      mod = find_constant(name)
      raise ArgumentError, "#{name} does not refer to a Module" unless mod.is_a?(Module)
      inner = read_value(ivars_consumed, partial: true)
      inner.extend(mod)
      inner
    end

    def read_user_class
      name = read_value
      klass = find_constant(name)
      inner = read_value
      if klass == Hash && inner.is_a?(Hash)
        inner.compare_by_identity
        return inner
      end
      raise ArgumentError, 'dump format error (user class)' unless klass.is_a?(Class)
      case inner
      when Hash
        raise ArgumentError, 'dump format error (user class)' unless klass <= Hash
        instance = klass.allocate
        instance.replace(inner)
        instance.compare_by_identity if inner.compare_by_identity?
        instance
      when Array
        raise ArgumentError, 'dump format error (user class)' unless klass <= Array
        klass.allocate.replace(inner)
      when String
        raise ArgumentError, 'dump format error (user class)' unless klass <= String
        klass.allocate.replace(inner)
      when Regexp
        raise ArgumentError, 'dump format error (user class)' unless klass <= Regexp
        klass.new(inner.source, inner.options)
      else
        raise ArgumentError, 'dump format error (user class)'
      end
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

    def read_user_marshaled_object_without_allocate(ivars_consumed = nil)
      name = read_value
      object_class = find_constant(name)
      raise TypeError, "#{object_class} needs to have method `_load'" unless object_class.respond_to?(:_load)
      data = read_string
      if ivars_consumed
        read_ivars(data)
        ivars_consumed[0] = true
      end
      object_class._load(data)
    end

    def read_struct
      name = read_value
      object_class = find_constant(name)
      size = read_integer
      values =
        size
          .times
          .each_with_object({}) do |_, tmp|
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
          __inline__ 'exception_var.as_exception_or_raise(env)->set_cause(cause_var.as_exception_or_raise(env));'
        end
      end
      ivars_hash.each { |ivar_name, value| object.instance_variable_set(ivar_name, value) }
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

    def read_value(ivars_consumed = nil, partial: false)
      char = read_byte.chr
      value =
        case char
        when '0'
          nil
        when 'T'
          true
        when 'F'
          false
        when '@'
          return read_object_link
        when 'i'
          read_integer
        when 'l'
          read_big_integer
        when ':'
          return read_symbol
        when ';'
          return read_symbol_link
        when 'f'
          read_float
        when 'I'
          ivars_consumed = [false]
          result = read_value(ivars_consumed, partial: true)
          read_ivars(result) unless result.is_a?(Regexp) || ivars_consumed[0]
          result
        else
          index = @object_lookup.size
          @object_lookup << nil # placeholder
          inner =
            case char
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
              read_user_marshaled_object_without_allocate(ivars_consumed)
            when 'S'
              read_struct
            when 'o'
              read_object
            when 'C'
              read_user_class
            when 'e'
              read_extended(ivars_consumed)
            else
              raise ArgumentError, 'dump format error'
            end
          @object_lookup[index] = inner
          inner
        end
      unless partial
        Kernel.instance_method(:freeze).bind_call(value) if @freeze && !value.is_a?(Module)
        value = @proc.call(value) if @proc
      end
      value
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
    def initialize(source, proc = nil, freeze: false)
      super(source, proc, freeze: freeze)
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
