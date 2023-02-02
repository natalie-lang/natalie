class Random
  class << self
    def rand(*args)
      DEFAULT.rand(*args)
    end

    def bytes(*args)
      Random.new.bytes(*args)
    end
  end
end
