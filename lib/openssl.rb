require 'natalie/inline'
require 'openssl.cpp'

__ld_flags__ '-lcrypto'

module OpenSSL
  module Random
    __bind_static_method__ :random_bytes :OpenSSL_Random_random_bytes
  end

  class Digest
    def self.digest(cipher, data)
      '' # TODO
    end

    def self.base64digest(cipher, data)
      [digest(cipher, data)].pack('m0')
    end

    def self.hexdigest(cipher, data)
      digest(cipher, data).unpack1('H*')
    end
  end
end
