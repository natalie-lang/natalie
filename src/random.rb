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
    if args.size == 1 && args[0].is_a?(Numeric) && args[0].negative?
      args[0] = 1.0
    end
    rand(*args)
  end
end
