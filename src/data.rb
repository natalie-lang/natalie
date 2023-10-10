class Data
  def self.define(*members, &block)
    members = members.map(&:to_sym)

    Class.new do
      members.each do |name|
        define_method(name) { instance_variable_get(:"@#{name}") }
      end

      define_method(:initialize) do |*args|
        if members.size != args.size
          raise ArgumentError, "wrong number of arguments (given #{args.size}, expected #{members.size})"
        end

        members.zip(args) do |name, value|
          instance_variable_set(:"@#{name}", value)
        end
      end

      define_singleton_method(:members) { members }

      if block
        instance_eval(&block)
      end
    end
  end
end
