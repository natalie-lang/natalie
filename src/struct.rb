class Struct
  include Enumerable

  class << self
    alias _original_new new
    alias [] new
  end

  def self.new(*attrs)
    if respond_to?(:members)
      _original_new(*attrs)
    else
      if attrs.last.is_a?(Hash)
        options = attrs.pop
      else
        options = {}
      end
      Class.new(Struct) do
        include Enumerable

        define_singleton_method :members do
          attrs
        end

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

        define_method :each do
          attrs.each { |attr| yield send(attr) }
        end

        define_method :inspect do
          str = '#<struct '
          attrs.each_with_index do |attr, index|
            str << "#{attr}=#{send(attr).inspect}"
            str << ', ' unless index == attrs.size - 1
          end
          str << '>'
        end

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
      end
    end
  end
end
