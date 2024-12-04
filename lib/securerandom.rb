require 'natalie/inline'
require 'securerandom.cpp'

require 'random/formatter'

class SecureRandom
  extend Random::Formatter

  __bind_static_method__ :random_number, :SecureRandom_random_number

  def self.bytes(n)
    Random.urandom(n)
  end
end
