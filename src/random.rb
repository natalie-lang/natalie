class Random
  class << self
    def rand(*args)
      DEFAULT.rand(*args)
    end
  end
end
