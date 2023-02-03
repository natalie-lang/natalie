class Random
  class << self
    def rand(*args)
      DEFAULT.rand(*args)
    end
    alias random_number rand

    def bytes(*args)
      Random.new.bytes(*args)
    end
  end

  alias random_number rand
end
