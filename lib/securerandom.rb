class SecureRandom
  class << self
    def base64(n=nil)
      [random_bytes(n)].pack('m0')
    end

    def bytes(n)
      Random.urandom(n)
    end

    def random_bytes(n=nil)
      Random.urandom(n ? n.to_int : 16)
    end
  end
end
