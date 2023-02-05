require 'natalie/inline'
require 'openssl.cpp'

__ld_flags__ '-lcrypto'

module OpenSSL
  module Random
    __bind_method__ :random_bytes :OpenSSL_Random_random_bytes

    # NATFIXME: __bind_method__ should support this directly
    def self.random_bytes(length)
      proxy = Object.new
      proxy.extend(self)
      proxy.random_bytes(length)
    end
  end
end
