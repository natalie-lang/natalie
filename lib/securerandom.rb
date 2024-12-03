require 'random/formatter'

class SecureRandom
  extend Random::Formatter

  def self.bytes(n)
    Random.urandom(n)
  end
end
