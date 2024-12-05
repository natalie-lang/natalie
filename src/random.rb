class Random
  class << self
    def rand(...)
      DEFAULT.rand(...)
    end

    def random_number(...)
      DEFAULT.random_number(...)
    end

    def bytes(...)
      Random.new.bytes(...)
    end
  end

  def random_number(*args)
    if args.size == 1
      if args[0].nil? || ((args[0].is_a?(Integer) || args[0].is_a?(Float)) && args[0].negative?)
        args[0] = 1.0
      elsif !args[0].is_a?(Numeric)
        raise ArgumentError, "invalid argument - #{args[0]}"
      end
    end
    rand(*args)
  end
end
