class Struct
  include Enumerable

  class << self
    alias_method :_original_new, :new
    alias_method :[], :new
  end

  def self.new(*attrs)
    if self == Struct
      if attrs.last.is_a?(Hash)
        options = attrs.pop
      else
        options = {}
      end
      Class.new(Struct) do
        include Enumerable

        define_method :length do
          attrs.length
        end
        alias_method :size, :length

        attrs.each { |attr| attr_accessor attr }

        if options[:keyword_init]
          define_method :initialize do |args|
            args.each { |attr, value| send("#{attr}=", value) }
          end
        else
          define_method :initialize do |*vals|
            attrs.each_with_index { |attr, index| send("#{attr}=", vals[index]) }
          end
        end

        self.class.define_method :keyword_init? do
          options[:keyword_init]
        end

        define_method :each do
          attrs.each { |attr| yield send(attr) }
        end

        alias_method :values :to_a

        define_method :inspect do
          str = '#<struct '
          attrs.each_with_index do |attr, index|
            str << "#{attr}=#{send(attr).inspect}"
            str << ', ' unless index == attrs.size - 1
          end
          str << '>'
        end
        alias_method :to_s, :inspect

        define_method(:deconstruct) do
          attrs.map { |attr| send(attr) }
        end

        define_method :[] do |arg|
          case arg
          when Integer
            attribute = attrs.fetch(arg)
            send(attribute)
          else
            send(arg)
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
      end
    else
      _original_new(*attrs)
    end
  end
end
