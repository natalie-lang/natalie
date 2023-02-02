class SecureRandom
  class << self
    def bytes(n)
      Random.urandom(n)
    end
  end
end
