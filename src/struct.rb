class Struct
  include Enumerable

  class << self
    alias [] new
  end

  def self.new(*attrs, keyword_init: nil, &block)
    duplicates = attrs.tally.find { |_, size| size > 1 }
    raise ArgumentError, "duplicate member: #{duplicates.first}" unless duplicates.nil?

    if !attrs.first.is_a?(Symbol) && attrs.first.respond_to?(:to_str)
      klass = attrs.shift.to_str.to_sym
    elsif attrs.first.nil?
      attrs.shift
    end

    result =
      Class.new(Struct) do
        include Enumerable

        define_singleton_method :members do
          attrs.dup
        end

        define_method :length do
          attrs.length
        end
        alias_method :size, :length

        if keyword_init
          define_method :initialize do |args = {}|
            raise ArgumentError, "wrong number of arguments (given #{args.size}, expected 0)" unless args.is_a?(Hash)
            invalid_keywords = args.each_key.reject { |arg| attrs.include?(arg) }
            raise ArgumentError, "unknown keywords: #{invalid_keywords.join(', ')}" unless invalid_keywords.empty?
            args.each { |attr, value| send("#{attr}=", value) }
          end
        else
          define_method :initialize do |*vals|
            raise ArgumentError, 'struct size differs' if vals.size > attrs.size
            attrs.each_with_index { |attr, index| send("#{attr}=", vals[index]) }
          end
        end

        self.class.define_method :keyword_init? do
          keyword_init ? true : keyword_init
        end

        define_method :each do
          if block_given?
            attrs.each { |attr| yield send(attr) }
            self
          else
            enum_for(:each) { length }
          end
        end

        define_method :each_pair do
          if block_given?
            attrs.each { |attr| yield [attr, send(attr)] }
            self
          else
            enum_for(:each_pair) { length }
          end
        end

        define_method :eql? do |other|
          self.class == other.class && values.zip(other.values).all? { |x, y| x.eql?(y) }
        end

        alias_method :values, :to_a

        define_method :inspect do
          inspected_attrs = attrs.map { |attr| "#{attr}=#{send(attr).inspect}" }
          "#<struct #{inspected_attrs.join(', ')}>"
        end
        alias_method :to_s, :inspect

        define_method(:deconstruct) { attrs.map { |attr| send(attr) } }

        define_method :deconstruct_keys do |keys|
          raise TypeError, "wrong argument type #{keys.class} (expected Array or nil)" if !keys.nil? && !keys.is_a?(Array)
          keys = members if keys.nil?
          next {} if keys.size > members.size
          result = {}
          keys.each do |key|
            key_sym = if key.is_a?(Symbol)
                        key
                      elsif key.is_a?(String)
                        key.to_sym
                      elsif key.is_a?(Integer)
                        members.fetch(key, nil)
                      elsif key.respond_to?(:to_int)
                        int_key = key.to_int
                        raise TypeError, "can't convert #{key.class} to Integer (#{key.class}#to_int gives #{int_key.class})" unless int_key.is_a?(Integer)
                        members.fetch(int_key, nil)
                      else
                        raise TypeError, "no implicit conversion of #{key.class} into Integer"
                      end
            break unless members.include?(key_sym)
            result[key] = public_send(key_sym)
          end
          result
        end

        define_method :dup do
          res =
            if self.class.keyword_init?
              self.class.new(to_h)
            else
              self.class.new(*to_a)
            end
          res.__send__(:initialize_dup, self)
        end

        define_method :[] do |arg, *rest|
          raise ArgumentError, 'too many arguments given' if rest.any?
          case arg
          when Integer
            arg = attrs.fetch(arg)
          when String, Symbol
            raise NameError, "no member '#{arg}' in struct" unless attrs.include?(arg.to_sym)
          end
          send(arg)
        end

        define_method :[]= do |key, value|
          raise FrozenError, "can't modify frozen #{self.class}: #{self}" if frozen?
          case key
          when String, Symbol
            raise NameError, "no member '#{key}' in struct" unless attrs.include?(key.to_sym)
          else
            key = attrs.fetch(key)
          end
          send("#{key}=", value)
        end

        define_method :== do |other|
          self.class == other.class && values == other.values
        end

        define_method :dig do |*args|
          raise ArgumentError, 'wrong number of arguments (given 0, expected 1+)' if args.empty?
          arg = args.shift
          res =
            begin
              self[arg]
            rescue StandardError
              nil
            end
          if args.empty? || res.nil?
            res
          elsif !res.respond_to?(:dig)
            raise TypeError, "#{res.class} does not have #dig method"
          else
            res.dig(*args)
          end
        end

        define_method :to_h do |&block|
          attrs.to_h do |attr|
            result = [attr.to_sym, send(attr)]
            result = block.call(*result) if block
            result
          end
        end

        define_method :members do
          attrs.dup
        end

        define_method :values_at do |*idxargs|
          result = []
          idxargs.each do |idx|
            case idx
            when Integer
              realidx = (idx < 0) ? idx + size : idx
              raise IndexError, "offset #{idx} too small for struct(size:#{size})" if realidx < 0
              raise IndexError, "offset #{idx} too large for struct(size:#{size})" if realidx >= size
              result << send(attrs.fetch(idx))
            when Range
              result.concat attrs.values_at(idx).map { |k| k.nil? ? nil : self[k] }
            else
              raise TypeError, "no implicit conversion of #{idx.class} into Integer"
            end
          end
          result
        end

        # These are generated for Struct.new(...).methods, but will be redefined
        # The actual values are not stored as ivars (requirement of Ruby specs)
        attrs.each { |attr| attr_accessor attr }

        instance_eval(&block) if block
      end

    if klass
      warn("warning: redefining constant Struct::#{klass}") if Struct.constants.include?(klass)
      Struct.const_set(klass, result)
    end

    def result.new(*args)
      object = allocate
      # The `values` variable will store the actual values of the struct. We redefine the
      # getter and setter methods in a closure to capture this variable and make it
      # invisible to the user.
      values = members.map { nil }
      members.each_with_index do |attr, idx|
        object.singleton_class.undef_method(attr)
        object.singleton_class.undef_method(:"#{attr}=")

        object.define_singleton_method(attr) { values[idx] }

        object.define_singleton_method(:"#{attr}=") do |value|
          raise FrozenError, "can't modify frozen #{self.class}: #{self}" if frozen?
          values[idx] = value
        end
      end
      object.__send__(:initialize, *args)
    end

    result
  end
end
