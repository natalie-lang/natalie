class Struct
  class << self
    alias_method :_original_new, :new
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
      end
    else
      _original_new(*attrs)
    end
  end
end
