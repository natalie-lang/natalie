class Struct
  include Enumerable

  class << self
    alias [] new
  end

  def self.new(*attrs, &block)
    duplicates = attrs.tally.find { |_, size| size > 1 }
    unless duplicates.nil?
      raise ArgumentError, "duplicate member: #{duplicates.first}"
    end

    if respond_to?(:members)
      BasicObject.method(:new).unbind.bind(self).(*attrs)
    else
      if attrs.empty?
        raise ArgumentError, 'wrong number of arguments (given 0, expected 1+)'
      end
      if !attrs.first.is_a?(Symbol) && attrs.first.respond_to?(:to_str)
        klass = attrs.shift.to_str.to_sym
      elsif attrs.first.nil?
        attrs.shift
      end

      if attrs.last.is_a?(Hash)
        options = attrs.pop
      else
        options = {}
      end
      result = Class.new(Struct) do
        include Enumerable

        define_singleton_method :members do
          attrs.dup
        end

        define_method :length do
          attrs.length
        end
        alias_method :size, :length

        if options[:keyword_init]
          define_method :initialize do |args = {}|
            unless args.is_a?(Hash)
              raise ArgumentError, "wrong number of arguments (given #{args.size}, expected 0)"
            end
            invalid_keywords = args.each_key.reject { |arg| attrs.include?(arg) }
            unless invalid_keywords.empty?
              raise ArgumentError, "unknown keywords: #{invalid_keywords.join(', ')}"
            end
            args.each do |attr, value|
              send("#{attr}=", value)
            end
          end
        else
          define_method :initialize do |*vals|
            if vals.size > attrs.size
              raise ArgumentError, 'struct size differs'
            end
            attrs.each_with_index { |attr, index| send("#{attr}=", vals[index]) }
          end
        end

        self.class.define_method :keyword_init? do
          options[:keyword_init]
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

        alias_method :values :to_a

        define_method :inspect do
          inspected_attrs = attrs.map { |attr| "#{attr}=#{send(attr).inspect}" }
          "#<struct #{inspected_attrs.join(', ')}>"
        end
        alias_method :to_s, :inspect

        define_method(:deconstruct) do
          attrs.map { |attr| send(attr) }
        end

        define_method :deconstruct_keys do |arg|
          if arg.nil?
            arg = attrs
          elsif !arg.is_a?(Array)
            raise TypeError, "wrong argument type #{arg.class} (expected Array or nil)"
          end

          if arg.size > attrs.size
            {}
          else
            arg = arg.take_while do |key|
              key.is_a?(Integer) ? key < attrs.size : attrs.include?(key.to_sym)
            end
            arg.each_with_object({}) do |key, memo|
              memo[key] = self[key]
            end
          end
        end

        define_method :[] do |arg|
          case arg
          when Integer
            arg = attrs.fetch(arg)
          when String, Symbol
            unless attrs.include?(arg.to_sym)
              raise NameError, "no member '#{arg}' in struct"
            end
          end
          send(arg)
        end

        define_method :[]= do |key, value|
          case key
          when String, Symbol
            unless attrs.include?(key.to_sym)
              raise NameError, "no member '#{key}' in struct"
            end
          else
            key = attrs.fetch(key)
          end
          send("#{key}=", value)
        end

        define_method :dig do |*args|
          if args.empty?
            raise ArgumentError, 'wrong number of arguments (given 0, expected 1+)'
          end
          arg = args.shift
          res = begin
                  self[arg]
                rescue
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
            if block
              result = block.call(*result)
            end
            result
          end
        end

        define_method :members do
          attrs.dup
        end

        attrs.each { |attr| attr_accessor attr }

        if block
          instance_eval(&block)
        end
      end

      if klass
        if Struct.constants.include?(klass)
          warn("warning: redefining constant Struct::#{klass}")
        end
        Struct.const_set(klass, result)
      end

      result
    end
  end
end
